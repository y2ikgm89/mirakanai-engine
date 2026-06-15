// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/core/diagnostics.hpp"
#include "mirakana/core/job_execution.hpp"
#include "mirakana/core/simd_dispatch.hpp"
#include "mirakana/environment/environment_io.hpp"
#include "mirakana/environment/environment_preset_pack.hpp"
#include "mirakana/environment/environment_quality_budget.hpp"
#include "mirakana/environment/weather_simulation.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/win32/win32_cpu_sets.hpp"
#include "mirakana/renderer/environment_lighting_policy.hpp"
#include "mirakana/renderer/environment_parity.hpp"
#include "mirakana/renderer/environment_performance.hpp"
#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/renderer/material_weathering_policy.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_game_host.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace {

struct DesktopRuntimeGameOptions {
    bool smoke{false};
    bool show_help{false};
    bool throttle{true};
    bool require_d3d12_scene_shaders{false};
    bool require_vulkan_scene_shaders{false};
    bool require_d3d12_renderer{false};
    bool require_vulkan_renderer{false};
    bool require_scene_gpu_bindings{false};
    bool require_postprocess{false};
    bool require_postprocess_depth_input{false};
    bool require_directional_shadow{false};
    bool require_directional_shadow_filtering{false};
    bool require_d3d12_shadow_cascade_policy{false};
    bool require_vulkan_shadow_cascade_policy{false};
    bool require_lighting_shadow_policy{false};
    bool require_scene_scale_policy{false};
    bool require_d3d12_instanced_draw_evidence{false};
    bool require_vulkan_instanced_draw_evidence{false};
    bool require_d3d12_postprocess_evidence{false};
    bool require_vulkan_postprocess_evidence{false};
    bool require_environment_profile{false};
    bool require_environment_fog_evidence{false};
    bool require_environment_fog_vulkan_package_evidence{false};
    bool require_physical_sky_package_evidence{false};
    bool require_physical_sky_vulkan_package_evidence{false};
    bool require_environment_volumetric_fog_package_evidence{false};
    bool require_environment_volumetric_fog_vulkan_renderer_execution{false};
    bool require_environment_lighting_package_evidence{false};
    bool require_environment_lighting_renderer_execution{false};
    bool require_environment_lighting_vulkan_renderer_execution{false};
    bool require_cloud_layer_package_evidence{false};
    bool require_cloud_layer_renderer_execution{false};
    bool require_environment_precipitation_package_evidence{false};
    bool require_environment_precipitation_renderer_execution{false};
    bool require_environment_precipitation_vulkan_renderer_execution{false};
    bool require_environment_snow_package_evidence{false};
    bool require_environment_snow_renderer_execution{false};
    bool require_environment_volumetric_cloud_package_evidence{false};
    bool require_environment_volumetric_cloud_renderer_execution{false};
    bool require_environment_volumetric_cloud_vulkan_renderer_execution{false};
    bool require_environment_material_weathering{false};
    bool require_environment_audio_playback{false};
    bool require_environment_texture_asset_pipeline_package{false};
    bool require_environment_preset_library_package{false};
    bool require_environment_ready_aggregate{false};
    bool require_environment_vulkan_strict_aggregate{false};
    bool require_environment_backend_parity{false};
    bool require_environment_platform_readiness{false};
    bool require_environment_optimization_measurement{false};
    bool require_environment_weather_simulation_package{false};
    bool require_gpu_memory_policy{false};
    bool require_memory_diagnostics{false};
    bool require_d3d12_gpu_memory_evidence{false};
    bool require_vulkan_gpu_memory_evidence{false};
    bool require_debug_profiling_policy{false};
    bool require_d3d12_debug_profiling_evidence{false};
    bool require_vulkan_debug_profiling_evidence{false};
    bool require_job_scheduling_evidence{false};
    bool require_job_execution_foundation{false};
    bool require_job_execution_topology_policy{false};
    bool require_job_execution_work_stealing{false};
    bool require_job_execution_placement_policy{false};
    bool require_windows_cpu_set_worker_placement{false};
    bool require_windows_cpu_set_smt_worker_placement{false};
    bool require_simd_dispatch_policy{false};
    bool require_renderer_quality_gates{false};
    bool require_framegraph_multiqueue_evidence{false};
    bool require_vulkan_framegraph_multiqueue_evidence{false};
    bool require_native_ui_overlay{false};
    bool require_native_ui_textured_sprite_atlas{false};
    bool require_gpu_skinning{false};
    bool require_d3d12_gpu_skinning_evidence{false};
    bool require_vulkan_gpu_skinning_evidence{false};
    bool require_quaternion_animation{false};
    std::uint32_t max_frames{0};
    std::string required_config_path;
    std::string required_scene_package_path;
};

constexpr std::string_view kExpectedConfigFormat{"format=GameEngine.SampleDesktopRuntimeGame.Config.v1"};
constexpr std::string_view kRuntimeSceneVertexShaderPath{"shaders/sample_desktop_runtime_game_scene.vs.dxil"};
constexpr std::string_view kRuntimeSceneSkinnedVertexShaderPath{
    "shaders/sample_desktop_runtime_game_scene_skinned.vs.dxil"};
constexpr std::string_view kRuntimeSceneFragmentShaderPath{"shaders/sample_desktop_runtime_game_scene.ps.dxil"};
constexpr std::string_view kRuntimeShadowReceiverFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_shadow_receiver.ps.dxil"};
constexpr std::string_view kRuntimeShadowReceiverSkinnedFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_shadow_receiver_skinned.ps.dxil"};
constexpr std::string_view kRuntimeSceneVulkanVertexShaderPath{"shaders/sample_desktop_runtime_game_scene.vs.spv"};
constexpr std::string_view kRuntimeSceneSkinnedVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_scene_skinned.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanFragmentShaderPath{"shaders/sample_desktop_runtime_game_scene.ps.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMappingShaderPath{
    "shaders/sample_desktop_runtime_game_scene_mapping.cs.spv"};
constexpr std::string_view kRuntimeShadowReceiverVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_shadow_receiver.ps.spv"};
constexpr std::string_view kRuntimeShadowReceiverSkinnedVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_shadow_receiver_skinned.ps.spv"};
constexpr std::string_view kRuntimePostprocessVertexShaderPath{
    "shaders/sample_desktop_runtime_game_postprocess.vs.dxil"};
constexpr std::string_view kRuntimePostprocessFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_postprocess.ps.dxil"};
constexpr std::string_view kRuntimeEnvironmentFogFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_environment_fog.ps.dxil"};
constexpr std::string_view kRuntimeEnvironmentFogVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_environment_fog.vs.spv"};
constexpr std::string_view kRuntimeEnvironmentFogVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_environment_fog.ps.spv"};
constexpr std::string_view kRuntimeEnvironmentVolumetricFogComputeShaderPath{
    "shaders/sample_desktop_runtime_game_environment_volumetric_fog.cs.dxil"};
constexpr std::string_view kRuntimeEnvironmentVolumetricFogVulkanComputeShaderPath{
    "shaders/sample_desktop_runtime_game_environment_volumetric_fog.cs.spv"};
constexpr std::string_view kRuntimeCloudLayerVertexShaderPath{
    "shaders/sample_desktop_runtime_game_cloud_layer.vs.dxil"};
constexpr std::string_view kRuntimeCloudLayerFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_cloud_layer.ps.dxil"};
constexpr std::string_view kRuntimePrecipitationVertexShaderPath{
    "shaders/sample_desktop_runtime_game_precipitation.vs.dxil"};
constexpr std::string_view kRuntimePrecipitationFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_precipitation.ps.dxil"};
constexpr std::string_view kRuntimePrecipitationVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_precipitation.vs.spv"};
constexpr std::string_view kRuntimePrecipitationVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_precipitation.ps.spv"};
constexpr std::string_view kRuntimeEnvironmentVolumetricCloudVertexShaderPath{
    "shaders/sample_desktop_runtime_game_environment_volumetric_cloud.vs.dxil"};
constexpr std::string_view kRuntimeEnvironmentVolumetricCloudFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_environment_volumetric_cloud.ps.dxil"};
constexpr std::string_view kRuntimeEnvironmentVolumetricCloudVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_environment_volumetric_cloud.vs.spv"};
constexpr std::string_view kRuntimeEnvironmentVolumetricCloudVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_environment_volumetric_cloud.ps.spv"};
constexpr std::string_view kRuntimeEnvironmentProfilePath{"runtime/assets/desktop_runtime/default_outdoor.geenv"};
constexpr std::string_view kRuntimeEnvironmentIblCubemapPath{
    "runtime/assets/desktop_runtime/environment_ibl.texture.geasset"};
constexpr std::string_view kRuntimeEnvironmentRadianceExrPath{
    "runtime/assets/desktop_runtime/environment_radiance_exr.texture.geasset"};
constexpr std::string_view kRuntimeEnvironmentSkyboxBasisPath{
    "runtime/assets/desktop_runtime/environment_skybox_basis.texture.geasset"};
constexpr std::string_view kRuntimeEnvironmentPresetPackPath{
    "runtime/assets/desktop_runtime/environment_presets.gepresetpack"};
constexpr std::string_view kRuntimeEnvironmentIblSampleVertexShaderPath{
    "shaders/sample_desktop_runtime_game_environment_ibl_sample.vs.dxil"};
constexpr std::string_view kRuntimeEnvironmentIblSampleFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_environment_ibl_sample.ps.dxil"};
constexpr std::string_view kRuntimeEnvironmentIblSampleVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_environment_ibl_sample.vs.spv"};
constexpr std::string_view kRuntimeEnvironmentIblSampleVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_environment_ibl_sample.ps.spv"};
constexpr std::string_view kRuntimePostprocessVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_postprocess.vs.spv"};
constexpr std::string_view kRuntimePostprocessVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_postprocess.ps.spv"};
constexpr std::string_view kRuntimeShadowVertexShaderPath{"shaders/sample_desktop_runtime_game_shadow.vs.dxil"};
constexpr std::string_view kRuntimeShadowFragmentShaderPath{"shaders/sample_desktop_runtime_game_shadow.ps.dxil"};
constexpr std::string_view kRuntimeShadowVulkanVertexShaderPath{"shaders/sample_desktop_runtime_game_shadow.vs.spv"};
constexpr std::string_view kRuntimeShadowVulkanFragmentShaderPath{"shaders/sample_desktop_runtime_game_shadow.ps.spv"};
constexpr std::string_view kRuntimeNativeUiOverlayVertexShaderPath{
    "shaders/sample_desktop_runtime_game_ui_overlay.vs.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_ui_overlay.ps.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_ui_overlay.vs.spv"};
constexpr std::string_view kRuntimeNativeUiOverlayVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_ui_overlay.ps.spv"};
constexpr std::string_view kHudAtlasProofResourceId{"hud.texture_atlas_proof"};
constexpr std::string_view kHudAtlasProofAssetUri{"runtime/assets/desktop_runtime/base_color.texture.geasset"};

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::CloudLayerPolicyDesc make_sample_cloud_layer_policy_desc(bool renderer_execution = false) {
    return mirakana::CloudLayerPolicyDesc{
        .layer =
            mirakana::EnvironmentCloudLayerDesc{
                .mode = mirakana::EnvironmentCloudLayerMode::equirectangular_2d,
                .coverage = 0.45F,
                .opacity = 0.8F,
                .altitude_m = 2400.0F,
                .wind_velocity_mps = mirakana::Vec2{.x = 8.0F, .y = 1.5F},
                .cloud_map_asset_ref = "sample/desktop-runtime/texture",
                .flow_map_asset_ref = "sample/desktop-runtime/cloud-flow",
                .sky_tint_response = mirakana::Vec3{.x = 0.78F, .y = 0.84F, .z = 0.92F},
                .time_of_day_response = 0.55F,
                .ibl_contribution_mode = mirakana::EnvironmentCloudIblContributionMode::sky_tint_only,
                .ibl_contribution = 0.25F,
            },
        .quality_tier = mirakana::CloudLayerQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
        .request_texture_upload = renderer_execution,
        .request_backend_execution = renderer_execution,
    };
}

[[nodiscard]] mirakana::PhysicalSkyPolicyDesc make_sample_physical_sky_policy_desc() {
    return mirakana::PhysicalSkyPolicyDesc{
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
    };
}

[[nodiscard]] mirakana::EnvironmentLightingPolicyDesc
make_sample_environment_lighting_policy_desc(bool package_record_ready, bool package_source_ready,
                                             bool hdr_metadata_ready) {
    return mirakana::EnvironmentLightingPolicyDesc{
        .dependency_mode = mirakana::EnvironmentLightingDependencyMode::first_party_cooked_texture,
        .visual_sky_source = mirakana::EnvironmentLightingSkySource::physical_sky,
        .lighting_sky_source = mirakana::EnvironmentLightingSkySource::specified_cubemap,
        .reflection_cubemap =
            mirakana::EnvironmentReflectionCubemapDesc{
                .asset = asset_id_from_game_asset_key("sample/desktop-runtime/environment/ibl-cubemap"),
                .edge_size = 16,
                .mip_count = 5,
                .format = mirakana::EnvironmentLightingTextureFormat::rgba16_float,
                .hdr_metadata_ready = hdr_metadata_ready,
                .package_record_ready = package_record_ready,
                .package_source_ready = package_source_ready,
                .package_evidence_ready = package_record_ready && package_source_ready && hdr_metadata_ready,
            },
        .irradiance =
            mirakana::EnvironmentIrradianceDesc{
                .spherical_harmonic_order = 3,
                .coefficient_count = 9,
            },
        .radiance =
            mirakana::EnvironmentRadianceDesc{
                .mip_count = 5,
                .roughness_mip_count = 5,
                .max_roughness = 1.0F,
            },
        .hdr_clamp =
            mirakana::EnvironmentHdrClampPolicyDesc{
                .enabled = true,
                .max_luminance_nits = 20000.0F,
                .exposure_bias = 0.0F,
            },
        .visual_sky_visible_in_main_camera = true,
    };
}

[[nodiscard]] mirakana::EnvironmentFogPolicyDesc make_sample_environment_fog_policy_desc() {
    return mirakana::EnvironmentFogPolicyDesc{
        .mode = mirakana::EnvironmentFogMode::exponential_height,
        .density = 0.08F,
        .height_falloff = 0.35F,
        .height_offset_m = 12.0F,
        .start_distance_m = 0.0F,
        .cutoff_distance_m = 1200.0F,
        .max_opacity = 0.85F,
        .sky_affect = 0.35F,
        .directional_inscattering_anisotropy = 0.25F,
        .inscattering_color = mirakana::Vec3{.x = 0.58F, .y = 0.68F, .z = 0.78F},
        .directional_inscattering_color = mirakana::Vec3{.x = 0.84F, .y = 0.78F, .z = 0.62F},
        .sample_step_budget = 8,
    };
}

[[nodiscard]] mirakana::VolumetricFogPolicyDesc
make_sample_environment_volumetric_fog_policy_desc(bool package_evidence_ready) {
    return mirakana::VolumetricFogPolicyDesc{
        .quality_tier = mirakana::VolumetricFogQualityTier::medium,
        .froxel_grid =
            mirakana::VolumetricFogFroxelGridDesc{
                .width = 160,
                .height = 90,
                .depth_slices = 32,
            },
        .range_m = 96.0F,
        .density = 0.045F,
        .albedo = mirakana::Vec3{.x = 0.82F, .y = 0.88F, .z = 0.94F},
        .anisotropy = 0.2F,
        .temporal =
            mirakana::VolumetricFogTemporalDesc{
                .enabled = true,
                .history_weight = 0.85F,
            },
        .raymarch_step_budget = 48,
        .shader_contract_evidence_ready = true,
        .execution_evidence_ready = true,
        .package_evidence_ready = package_evidence_ready,
        .request_ready_promotion = true,
    };
}

[[nodiscard]] mirakana::PrecipitationPolicyDesc make_sample_environment_precipitation_policy_desc() {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = "sample_desktop_runtime_precipitation";
    profile.weather = mirakana::EnvironmentWeatherKind::storm;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = mirakana::EnvironmentPrecipitationKind::rain,
        .intensity = 0.8F,
        .particle_radius_mm = 0.8F,
        .fall_speed_mps = 8.5F,
        .wind_speed_mps = 6.0F,
    };
    return mirakana::PrecipitationPolicyDesc{
        .environment_plan = mirakana::plan_environment_precipitation(mirakana::EnvironmentPrecipitationPlanDesc{
            .environment = profile,
            .scene_geometry_occlusion_required = true,
            .occlusion_policy_available = true,
        }),
        .quality_tier = mirakana::PrecipitationQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
    };
}

[[nodiscard]] mirakana::PrecipitationPolicyDesc make_sample_environment_snow_policy_desc() {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = "sample_desktop_runtime_snow";
    profile.weather = mirakana::EnvironmentWeatherKind::snow;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = mirakana::EnvironmentPrecipitationKind::snow,
        .intensity = 0.55F,
        .particle_radius_mm = 1.1F,
        .fall_speed_mps = 1.4F,
        .wind_speed_mps = 2.0F,
    };
    return mirakana::PrecipitationPolicyDesc{
        .environment_plan = mirakana::plan_environment_precipitation(mirakana::EnvironmentPrecipitationPlanDesc{
            .environment = profile,
            .scene_geometry_occlusion_required = true,
            .occlusion_policy_available = true,
        }),
        .quality_tier = mirakana::PrecipitationQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
    };
}

[[nodiscard]] mirakana::MaterialWeatheringPolicyDesc make_sample_environment_material_weathering_policy_desc(
    bool request_ready_promotion, std::uint64_t material_parameter_bindings, std::uint64_t material_constant_bytes,
    std::uint64_t backend_invocations) {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = "sample_desktop_runtime_material_weathering";
    profile.weather = mirakana::EnvironmentWeatherKind::storm;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = mirakana::EnvironmentPrecipitationKind::rain,
        .intensity = 0.75F,
        .particle_radius_mm = 0.8F,
        .fall_speed_mps = 8.5F,
        .wind_speed_mps = 6.0F,
    };
    const bool execution_ready = request_ready_promotion && material_parameter_bindings > 0U &&
                                 material_constant_bytes > 0U && backend_invocations > 0U;
    return mirakana::MaterialWeatheringPolicyDesc{
        .environment_plan = mirakana::plan_environment_material_weathering(mirakana::EnvironmentMaterialWeatheringDesc{
            .environment = profile,
            .snow_accumulation = 0.5F,
            .ice_intensity = 0.2F,
        }),
        .quality_tier = mirakana::MaterialWeatheringQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = execution_ready,
        .request_ready_promotion = request_ready_promotion,
        .request_material_parameter_binding = request_ready_promotion,
        .request_backend_execution = request_ready_promotion,
        .material_parameter_binding_count = material_parameter_bindings,
        .material_constant_bytes_uploaded = material_constant_bytes,
        .backend_invocation_count = backend_invocations,
    };
}

[[nodiscard]] std::span<const mirakana::VolumetricCloudAtmosphericLightDesc>
sample_environment_volumetric_cloud_lights() noexcept {
    static constexpr std::array lights{
        mirakana::VolumetricCloudAtmosphericLightDesc{
            .direction = mirakana::Vec3{.x = 0.3F, .y = -1.0F, .z = 0.1F},
            .color = mirakana::Vec3{.x = 1.0F, .y = 0.95F, .z = 0.85F},
            .illuminance_lux = 100000.0F,
            .casts_cloud_shadows = true,
            .source_index = 0U,
        },
    };
    return lights;
}

[[nodiscard]] mirakana::VolumetricCloudPolicyDesc make_sample_environment_volumetric_cloud_policy_desc() {
    return mirakana::VolumetricCloudPolicyDesc{
        .layer =
            mirakana::VolumetricCloudLayerDesc{
                .weather_map_asset_ref = "sample/desktop-runtime/texture",
                .shape_noise_asset_ref = "sample/desktop-runtime/volumetric-cloud-shape-noise-3d",
                .erosion_noise_asset_ref = "sample/desktop-runtime/volumetric-cloud-erosion-noise-3d",
                .coverage = 0.82F,
                .density = 0.92F,
                .altitude_min_m = 1000.0F,
                .altitude_max_m = 4000.0F,
                .wind_velocity_mps = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            },
        .quality_tier = mirakana::VolumetricCloudQualityTier::balanced,
        .raymarch =
            mirakana::VolumetricCloudRaymarchBudgetDesc{
                .primary_steps = 48U,
                .light_steps = 8U,
                .shadow_mode = mirakana::VolumetricCloudShadowMode::beer_shadow_map_intent,
                .temporal_reprojection_enabled = true,
                .temporal_history_weight = 0.9F,
            },
        .atmospheric_lights = sample_environment_volumetric_cloud_lights(),
        .storm =
            mirakana::VolumetricCloudStormDesc{
                .enabled = true,
                .lightning_flash_intensity = 10000.0F,
                .lightning_direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
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

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/scene");
}

[[nodiscard]] mirakana::AssetId packaged_ui_atlas_metadata_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/ui-atlas-metadata");
}

[[nodiscard]] mirakana::AssetId packaged_quaternion_animation_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/animations/packaged-pose");
}

[[nodiscard]] mirakana::AssetId packaged_environment_profile_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/environment/default-outdoor");
}

[[nodiscard]] mirakana::AssetId packaged_environment_ibl_cubemap_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/environment/ibl-cubemap");
}

[[nodiscard]] mirakana::AssetId packaged_environment_radiance_exr_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/environment/radiance-exr");
}

[[nodiscard]] mirakana::AssetId packaged_environment_skybox_basis_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/environment/skybox-basis");
}

[[nodiscard]] mirakana::AssetId packaged_environment_preset_pack_asset_id() {
    return asset_id_from_game_asset_key("environment/presets/sample_commercial_pack");
}

[[nodiscard]] mirakana::AssetId packaged_material_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/material");
}

[[nodiscard]] mirakana::AnimationSkeleton3dDesc packaged_quaternion_animation_skeleton() {
    return mirakana::AnimationSkeleton3dDesc{
        {mirakana::AnimationSkeleton3dJointDesc{.name = "PackagedMesh", .parent_index = mirakana::animation_no_parent}},
    };
}

[[nodiscard]] std::vector<mirakana::AnimationJointTrack3dDesc>
make_quaternion_animation_tracks(const mirakana::runtime::RuntimeAnimationQuaternionClipPayload& payload) {
    std::vector<mirakana::AnimationJointTrack3dByteSource> sources;
    sources.reserve(payload.clip.tracks.size());
    for (const auto& track : payload.clip.tracks) {
        sources.push_back(mirakana::AnimationJointTrack3dByteSource{
            .joint_index = track.joint_index,
            .target = track.target,
            .translation_time_seconds_bytes = track.translation_time_seconds_bytes,
            .translation_xyz_bytes = track.translation_xyz_bytes,
            .rotation_time_seconds_bytes = track.rotation_time_seconds_bytes,
            .rotation_xyzw_bytes = track.rotation_xyzw_bytes,
            .scale_time_seconds_bytes = track.scale_time_seconds_bytes,
            .scale_xyz_bytes = track.scale_xyz_bytes,
        });
    }
    return mirakana::make_animation_joint_tracks_3d_from_f32_bytes(sources);
}

constexpr mirakana::SceneNodeId kPackagedMeshNode{1};
constexpr mirakana::SceneNodeId kPrimaryCameraNode{3};

enum class UiAtlasMetadataStatus : std::uint8_t {
    not_requested,
    missing,
    malformed,
    invalid_reference,
    unsupported,
    ready,
};

struct UiAtlasMetadataRuntimeState {
    UiAtlasMetadataStatus status{UiAtlasMetadataStatus::not_requested};
    mirakana::UiRendererImagePalette palette;
    mirakana::AssetId atlas_page;
    std::size_t pages{0};
    std::size_t bindings{0};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view ui_atlas_metadata_status_name(UiAtlasMetadataStatus status) noexcept {
    switch (status) {
    case UiAtlasMetadataStatus::not_requested:
        return "not_requested";
    case UiAtlasMetadataStatus::missing:
        return "missing";
    case UiAtlasMetadataStatus::malformed:
        return "malformed";
    case UiAtlasMetadataStatus::invalid_reference:
        return "invalid_reference";
    case UiAtlasMetadataStatus::unsupported:
        return "unsupported";
    case UiAtlasMetadataStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] UiAtlasMetadataStatus
classify_ui_atlas_metadata_failure(mirakana::AssetId metadata_asset,
                                   const mirakana::UiRendererImagePaletteBuildFailure& failure) {
    if (failure.asset != metadata_asset) {
        return UiAtlasMetadataStatus::invalid_reference;
    }
    if (failure.diagnostic.find("source image decoding") != std::string::npos ||
        failure.diagnostic.find("production atlas packing") != std::string::npos) {
        return UiAtlasMetadataStatus::unsupported;
    }
    if (failure.diagnostic.find("missing") != std::string::npos) {
        return UiAtlasMetadataStatus::missing;
    }
    return UiAtlasMetadataStatus::malformed;
}

[[nodiscard]] UiAtlasMetadataRuntimeState
load_required_ui_atlas_metadata(const mirakana::runtime::RuntimeAssetPackage* package) {
    UiAtlasMetadataRuntimeState state;
    const auto metadata_asset = packaged_ui_atlas_metadata_asset_id();
    if (package == nullptr) {
        state.status = UiAtlasMetadataStatus::missing;
        state.diagnostics.push_back("ui atlas metadata requires a loaded runtime package");
        return state;
    }

    auto result = mirakana::build_ui_renderer_image_palette_from_runtime_ui_atlas(*package, metadata_asset);
    if (!result.succeeded()) {
        state.status = UiAtlasMetadataStatus::malformed;
        for (const auto& failure : result.failures) {
            const auto classified = classify_ui_atlas_metadata_failure(metadata_asset, failure);
            if (state.status == UiAtlasMetadataStatus::malformed || classified == UiAtlasMetadataStatus::unsupported ||
                classified == UiAtlasMetadataStatus::invalid_reference) {
                state.status = classified;
            }
            state.diagnostics.push_back(failure.diagnostic);
        }
        return state;
    }

    state.status = UiAtlasMetadataStatus::ready;
    state.pages = result.atlas_page_assets.size();
    state.bindings = result.palette.count();
    state.atlas_page = result.atlas_page_assets.empty() ? mirakana::AssetId{} : result.atlas_page_assets.front();
    state.palette = std::move(result.palette);
    if (state.pages == 0 || state.bindings == 0 || state.atlas_page.value == 0) {
        state.status = UiAtlasMetadataStatus::malformed;
        state.diagnostics.push_back("ui atlas metadata did not produce any atlas page or image binding");
    }
    return state;
}

void print_ui_atlas_metadata_diagnostics(std::string_view prefix, const UiAtlasMetadataRuntimeState& state) {
    for (const auto& diagnostic : state.diagnostics) {
        std::cout << prefix << " ui_atlas_metadata_diagnostic=" << ui_atlas_metadata_status_name(state.status) << ": "
                  << diagnostic << '\n';
    }
}

enum class EnvironmentProfilePackageStatus : std::uint8_t {
    not_requested,
    missing_package,
    missing_profile_record,
    invalid_profile_record,
    missing_scene_reference,
    invalid_scene_reference,
    missing_scene_dependency,
    missing_dependency_edge,
    runtime_scene_validation_failed,
    ready,
};

struct EnvironmentProfilePackageEvidence {
    EnvironmentProfilePackageStatus status{EnvironmentProfilePackageStatus::not_requested};
    bool requested{false};
    bool ready{false};
    bool package_file_present{false};
    bool package_index_entry_present{false};
    bool scene_reference_present{false};
    bool scene_required{false};
    bool scene_dependency_present{false};
    bool dependency_edge_present{false};
    bool runtime_source_parsing{false};
    bool legacy_v1_accepted{false};
    std::size_t volume_rows{0};
    std::size_t weather_keyframes{0};
    std::string quality_preset{"unknown"};
    std::size_t diagnostics{0};
};

[[nodiscard]] std::string_view
environment_profile_package_status_name(EnvironmentProfilePackageStatus status) noexcept {
    switch (status) {
    case EnvironmentProfilePackageStatus::not_requested:
        return "not_requested";
    case EnvironmentProfilePackageStatus::missing_package:
        return "missing_package";
    case EnvironmentProfilePackageStatus::missing_profile_record:
        return "missing_profile_record";
    case EnvironmentProfilePackageStatus::invalid_profile_record:
        return "invalid_profile_record";
    case EnvironmentProfilePackageStatus::missing_scene_reference:
        return "missing_scene_reference";
    case EnvironmentProfilePackageStatus::invalid_scene_reference:
        return "invalid_scene_reference";
    case EnvironmentProfilePackageStatus::missing_scene_dependency:
        return "missing_scene_dependency";
    case EnvironmentProfilePackageStatus::missing_dependency_edge:
        return "missing_dependency_edge";
    case EnvironmentProfilePackageStatus::runtime_scene_validation_failed:
        return "runtime_scene_validation_failed";
    case EnvironmentProfilePackageStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] bool
has_environment_profile_dependency_edge(const mirakana::runtime::RuntimeAssetPackage& package) noexcept {
    const auto scene_asset = packaged_scene_asset_id();
    const auto environment_asset = packaged_environment_profile_asset_id();
    return std::ranges::any_of(package.dependency_edges(), [scene_asset, environment_asset](const auto& edge) {
        return edge.asset == scene_asset && edge.dependency == environment_asset &&
               edge.kind == mirakana::AssetDependencyKind::scene_environment_profile;
    });
}

[[nodiscard]] bool
has_environment_profile_scene_dependency(const mirakana::runtime::RuntimeAssetPackage& package) noexcept {
    const auto* scene_record = package.find(packaged_scene_asset_id());
    if (scene_record == nullptr) {
        return false;
    }
    const auto environment_asset = packaged_environment_profile_asset_id();
    return std::ranges::find(scene_record->dependencies, environment_asset) != scene_record->dependencies.end();
}

[[nodiscard]] bool is_cooked_environment_profile_record(const mirakana::runtime::RuntimeAssetRecord& record) noexcept {
    return record.kind == mirakana::AssetKind::environment_profile && record.path == kRuntimeEnvironmentProfilePath &&
           std::string_view{record.content}.starts_with("format=GameEngine.CookedEnvironmentProfile.v2\n") &&
           record.content.find("asset.kind=environment_profile\n") != std::string::npos &&
           record.content.find("environment.source_format=GameEngine.EnvironmentProfile.v2\n") != std::string::npos &&
           record.content.find("profile_v2.global.id=default_outdoor\n") != std::string::npos;
}

[[nodiscard]] std::size_t count_environment_profile_rows(std::string_view content,
                                                         std::string_view row_prefix) noexcept {
    std::size_t count{0};
    for (;;) {
        const auto key = std::string{"profile_v2."} + std::string{row_prefix} + "." + std::to_string(count) + ".";
        if (content.find(key) == std::string_view::npos) {
            return count;
        }
        ++count;
    }
}

[[nodiscard]] std::string environment_profile_quality_preset(std::string_view content) {
    constexpr std::string_view key{"profile_v2.quality.preset="};
    const auto begin = content.find(key);
    if (begin == std::string_view::npos) {
        return "missing";
    }
    const auto value_begin = begin + key.size();
    const auto value_end = content.find('\n', value_begin);
    return std::string{
        content.substr(value_begin, value_end == std::string_view::npos ? value_end : value_end - value_begin)};
}

[[nodiscard]] bool accepts_legacy_environment_profile_v1(std::string_view content) noexcept {
    return content.starts_with("format=GameEngine.CookedEnvironmentProfile.v1\n") ||
           content.find("environment.source_format=GameEngine.EnvironmentProfile.v1\n") != std::string_view::npos;
}

[[nodiscard]] EnvironmentProfilePackageEvidence
evaluate_environment_profile_package(bool requested,
                                     const std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package) {
    EnvironmentProfilePackageEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }
    if (!runtime_package.has_value()) {
        evidence.status = EnvironmentProfilePackageStatus::missing_package;
        evidence.diagnostics = 1;
        return evidence;
    }

    const auto environment_asset = packaged_environment_profile_asset_id();
    const auto* environment_record = runtime_package->find(environment_asset);
    evidence.package_index_entry_present = environment_record != nullptr;
    if (environment_record == nullptr) {
        evidence.status = EnvironmentProfilePackageStatus::missing_profile_record;
        evidence.diagnostics = 1;
        return evidence;
    }
    evidence.package_file_present = is_cooked_environment_profile_record(*environment_record);
    if (!evidence.package_file_present) {
        evidence.status = EnvironmentProfilePackageStatus::invalid_profile_record;
        evidence.diagnostics = 1;
        return evidence;
    }
    evidence.volume_rows = count_environment_profile_rows(environment_record->content, "volume");
    evidence.weather_keyframes = count_environment_profile_rows(environment_record->content, "weather_keyframe");
    evidence.quality_preset = environment_profile_quality_preset(environment_record->content);
    evidence.legacy_v1_accepted = accepts_legacy_environment_profile_v1(environment_record->content);
    if (evidence.volume_rows == 0U || evidence.weather_keyframes == 0U || evidence.quality_preset == "missing" ||
        evidence.legacy_v1_accepted) {
        evidence.status = EnvironmentProfilePackageStatus::invalid_profile_record;
        evidence.diagnostics = 1;
        return evidence;
    }

    const auto runtime_scene = mirakana::runtime_scene::instantiate_runtime_scene(
        *runtime_package, packaged_scene_asset_id(),
        mirakana::runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = true,
                                                         .require_unique_node_names = false,
                                                         .require_environment_profile = true});
    evidence.diagnostics = runtime_scene.diagnostics.size();
    if (!runtime_scene.succeeded() || !runtime_scene.instance.has_value()) {
        evidence.status = EnvironmentProfilePackageStatus::runtime_scene_validation_failed;
        return evidence;
    }

    const auto environment = runtime_scene.instance->scene.environment();
    evidence.scene_reference_present = environment.has_value();
    if (!environment.has_value()) {
        evidence.status = EnvironmentProfilePackageStatus::missing_scene_reference;
        evidence.diagnostics += 1;
        return evidence;
    }
    evidence.scene_required = environment->required;
    if (environment->profile != environment_asset || !environment->required) {
        evidence.status = EnvironmentProfilePackageStatus::invalid_scene_reference;
        evidence.diagnostics += 1;
        return evidence;
    }

    evidence.scene_dependency_present = has_environment_profile_scene_dependency(*runtime_package);
    if (!evidence.scene_dependency_present) {
        evidence.status = EnvironmentProfilePackageStatus::missing_scene_dependency;
        evidence.diagnostics += 1;
        return evidence;
    }
    evidence.dependency_edge_present = has_environment_profile_dependency_edge(*runtime_package);
    if (!evidence.dependency_edge_present) {
        evidence.status = EnvironmentProfilePackageStatus::missing_dependency_edge;
        evidence.diagnostics += 1;
        return evidence;
    }

    evidence.status = EnvironmentProfilePackageStatus::ready;
    evidence.ready = true;
    return evidence;
}

enum class EnvironmentTextureAssetPipelinePackageStatus : std::uint8_t {
    not_requested,
    missing_package,
    missing_texture_record,
    invalid_texture_record,
    missing_profile_record,
    missing_profile_dependency,
    missing_dependency_edge,
    ready,
};

enum class EnvironmentPresetLibraryPackageStatus : std::uint8_t {
    not_requested,
    missing_package,
    missing_pack_record,
    invalid_pack_record,
    missing_dependency_ref,
    ready,
};

struct EnvironmentTextureAssetPipelinePackageEvidence {
    EnvironmentTextureAssetPipelinePackageStatus status{EnvironmentTextureAssetPipelinePackageStatus::not_requested};
    bool requested{false};
    bool ready{false};
    std::size_t package_index_entries{0};
    std::size_t metadata_records{0};
    std::size_t metadata_only_records{0};
    std::size_t openexr_records{0};
    std::size_t ktx2_basis_records{0};
    std::size_t source_hash_rows{0};
    std::size_t provenance_rows{0};
    std::size_t license_rows{0};
    std::size_t backend_policy_rows{0};
    std::size_t unsupported_host_diagnostics{0};
    std::size_t environment_profile_dependency_refs{0};
    std::size_t environment_texture_dependency_edges{0};
    bool pixel_decode_invoked{false};
    bool basis_runtime_transcode_invoked{false};
    bool gpu_upload_invoked{false};
    bool broad_asset_pipeline_ready{false};
    std::size_t diagnostics{0};
};

struct EnvironmentPresetLibraryPackageEvidence {
    EnvironmentPresetLibraryPackageStatus status{EnvironmentPresetLibraryPackageStatus::not_requested};
    bool requested{false};
    bool ready{false};
    bool package_index_entry{false};
    bool package_file{false};
    std::size_t preset_count{0};
    std::size_t required_preset_rows{0};
    std::size_t backend_feature_rows{0};
    std::size_t dependency_refs{0};
    bool sample_consumption_evidence{false};
    bool aaa_ready_claimed{false};
    std::size_t diagnostics{0};
};

[[nodiscard]] std::string_view
environment_texture_asset_pipeline_package_status_name(EnvironmentTextureAssetPipelinePackageStatus status) noexcept {
    switch (status) {
    case EnvironmentTextureAssetPipelinePackageStatus::not_requested:
        return "not_requested";
    case EnvironmentTextureAssetPipelinePackageStatus::missing_package:
        return "missing_package";
    case EnvironmentTextureAssetPipelinePackageStatus::missing_texture_record:
        return "missing_texture_record";
    case EnvironmentTextureAssetPipelinePackageStatus::invalid_texture_record:
        return "invalid_texture_record";
    case EnvironmentTextureAssetPipelinePackageStatus::missing_profile_record:
        return "missing_profile_record";
    case EnvironmentTextureAssetPipelinePackageStatus::missing_profile_dependency:
        return "missing_profile_dependency";
    case EnvironmentTextureAssetPipelinePackageStatus::missing_dependency_edge:
        return "missing_dependency_edge";
    case EnvironmentTextureAssetPipelinePackageStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
environment_preset_library_package_status_name(EnvironmentPresetLibraryPackageStatus status) noexcept {
    switch (status) {
    case EnvironmentPresetLibraryPackageStatus::not_requested:
        return "not_requested";
    case EnvironmentPresetLibraryPackageStatus::missing_package:
        return "missing_package";
    case EnvironmentPresetLibraryPackageStatus::missing_pack_record:
        return "missing_pack_record";
    case EnvironmentPresetLibraryPackageStatus::invalid_pack_record:
        return "invalid_pack_record";
    case EnvironmentPresetLibraryPackageStatus::missing_dependency_ref:
        return "missing_dependency_ref";
    case EnvironmentPresetLibraryPackageStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] std::optional<std::uint32_t> parse_metadata_u32(std::string_view content, std::string_view key) noexcept {
    const auto begin = content.find(key);
    if (begin == std::string_view::npos) {
        return std::nullopt;
    }
    const auto value_begin = begin + key.size();
    const auto value_end = content.find('\n', value_begin);
    const auto value = content.substr(value_begin, value_end == std::string_view::npos ? std::string_view::npos
                                                                                       : value_end - value_begin);
    std::uint32_t parsed{};
    const auto* const first = value.data();
    const auto* const last = value.data() + value.size();
    const auto result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc{} || result.ptr != last) {
        return std::nullopt;
    }
    return parsed;
}

[[nodiscard]] std::size_t metadata_backend_policy_rows(std::string_view content) noexcept {
    const auto count = parse_metadata_u32(content, "texture.backend_policy_count=");
    return count.has_value() ? *count : 0U;
}

[[nodiscard]] std::size_t metadata_unsupported_host_diagnostics(std::string_view content) noexcept {
    const auto count = parse_metadata_u32(content, "texture.unsupported_host_diagnostic_count=");
    return count.has_value() ? *count : 0U;
}

[[nodiscard]] bool has_record_dependency(const mirakana::runtime::RuntimeAssetRecord& record,
                                         mirakana::AssetId dependency) noexcept {
    return std::ranges::find(record.dependencies, dependency) != record.dependencies.end();
}

[[nodiscard]] bool has_environment_texture_dependency_edge(const mirakana::runtime::RuntimeAssetPackage& package,
                                                           mirakana::AssetId texture) noexcept {
    const auto environment_asset = packaged_environment_profile_asset_id();
    return std::ranges::any_of(package.dependency_edges(), [environment_asset, texture](const auto& edge) {
        return edge.asset == environment_asset && edge.dependency == texture &&
               edge.kind == mirakana::AssetDependencyKind::environment_texture &&
               edge.path == kRuntimeEnvironmentProfilePath;
    });
}

[[nodiscard]] bool is_environment_texture_metadata_record(const mirakana::runtime::RuntimeAssetRecord& record,
                                                          std::string_view expected_path,
                                                          std::string_view expected_source_kind) noexcept {
    const std::string_view content{record.content};
    return record.kind == mirakana::AssetKind::texture && record.path == expected_path &&
           content.starts_with("format=GameEngine.EnvironmentTextureGeassetMetadata.v1\n") &&
           content.find("asset.kind=environment_texture\n") != std::string_view::npos &&
           content.find("asset.payload=metadata_only\n") != std::string_view::npos &&
           content.find("source.format=GameEngine.TextureSource.v2\n") != std::string_view::npos &&
           content.find(std::string{"source.kind="} + std::string{expected_source_kind} + "\n") !=
               std::string_view::npos &&
           content.find("source.hash=sha256:") != std::string_view::npos &&
           content.find("source.provenance_id=provenance.environment.") != std::string_view::npos &&
           content.find("source.license_id=LicenseRef-Proprietary\n") != std::string_view::npos &&
           metadata_backend_policy_rows(content) == 5U && metadata_unsupported_host_diagnostics(content) == 4U;
}

[[nodiscard]] bool
contains_required_environment_preset(std::span<const mirakana::EnvironmentPresetPackPresetV1> presets,
                                     std::string_view id) noexcept {
    return std::ranges::any_of(presets, [id](const auto& preset) { return preset.id == id; });
}

[[nodiscard]] std::size_t
count_required_environment_presets(std::span<const mirakana::EnvironmentPresetPackPresetV1> presets) noexcept {
    constexpr std::array required_ids{
        std::string_view{"clear_noon"},
        std::string_view{"overcast_storm"},
        std::string_view{"night_moonlit"},
        std::string_view{"snowfield"},
        std::string_view{"foggy_valley"},
        std::string_view{"cinematic_sunset"},
        std::string_view{"indoor_to_outdoor_transition"},
    };

    return static_cast<std::size_t>(std::ranges::count_if(
        required_ids, [presets](std::string_view id) { return contains_required_environment_preset(presets, id); }));
}

[[nodiscard]] bool is_environment_preset_pack_record(const mirakana::runtime::RuntimeAssetRecord& record) noexcept {
    return record.kind == mirakana::AssetKind::environment_preset_pack &&
           record.path == kRuntimeEnvironmentPresetPackPath &&
           std::string_view{record.content}.starts_with("format=GameEngine.EnvironmentPresetPack.v1\n");
}

[[nodiscard]] EnvironmentPresetLibraryPackageEvidence evaluate_environment_preset_library_package(
    bool requested, const std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package) {
    EnvironmentPresetLibraryPackageEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }
    if (!runtime_package.has_value()) {
        evidence.status = EnvironmentPresetLibraryPackageStatus::missing_package;
        evidence.diagnostics = 1U;
        return evidence;
    }

    const auto* record = runtime_package->find(packaged_environment_preset_pack_asset_id());
    if (record == nullptr) {
        evidence.status = EnvironmentPresetLibraryPackageStatus::missing_pack_record;
        evidence.diagnostics = 1U;
        return evidence;
    }

    evidence.package_index_entry = true;
    evidence.package_file = is_environment_preset_pack_record(*record);
    if (!evidence.package_file) {
        evidence.status = EnvironmentPresetLibraryPackageStatus::invalid_pack_record;
        evidence.diagnostics = 1U;
        return evidence;
    }

    const auto document = mirakana::deserialize_environment_preset_pack_v1(record->content);
    const auto validation = mirakana::validate_environment_preset_pack_v1(document);
    evidence.preset_count = document.presets.size();
    evidence.required_preset_rows = count_required_environment_presets(document.presets);
    evidence.backend_feature_rows = document.required_backend_feature_rows.size();
    evidence.dependency_refs += has_record_dependency(*record, packaged_environment_profile_asset_id()) ? 1U : 0U;
    evidence.dependency_refs += has_record_dependency(*record, packaged_environment_radiance_exr_asset_id()) ? 1U : 0U;
    evidence.dependency_refs += has_record_dependency(*record, packaged_environment_skybox_basis_asset_id()) ? 1U : 0U;
    evidence.sample_consumption_evidence = validation.succeeded() && evidence.preset_count == 7U &&
                                           evidence.required_preset_rows == 7U && evidence.backend_feature_rows >= 1U &&
                                           evidence.dependency_refs == 3U;
    if (!evidence.sample_consumption_evidence) {
        evidence.status = evidence.dependency_refs == 3U
                              ? EnvironmentPresetLibraryPackageStatus::invalid_pack_record
                              : EnvironmentPresetLibraryPackageStatus::missing_dependency_ref;
        evidence.diagnostics = 1U;
        return evidence;
    }

    evidence.status = EnvironmentPresetLibraryPackageStatus::ready;
    evidence.ready = evidence.package_index_entry && evidence.package_file && evidence.sample_consumption_evidence &&
                     !evidence.aaa_ready_claimed;
    if (!evidence.ready) {
        evidence.status = EnvironmentPresetLibraryPackageStatus::invalid_pack_record;
        evidence.diagnostics = 1U;
    }
    return evidence;
}

[[nodiscard]] EnvironmentTextureAssetPipelinePackageEvidence evaluate_environment_texture_asset_pipeline_package(
    bool requested, const std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package) {
    EnvironmentTextureAssetPipelinePackageEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }
    if (!runtime_package.has_value()) {
        evidence.status = EnvironmentTextureAssetPipelinePackageStatus::missing_package;
        evidence.diagnostics = 1U;
        return evidence;
    }

    const auto* environment_record = runtime_package->find(packaged_environment_profile_asset_id());
    if (environment_record == nullptr) {
        evidence.status = EnvironmentTextureAssetPipelinePackageStatus::missing_profile_record;
        evidence.diagnostics = 1U;
        return evidence;
    }

    const std::array expected_records{
        std::tuple{packaged_environment_radiance_exr_asset_id(), kRuntimeEnvironmentRadianceExrPath,
                   std::string_view{"openexr"}},
        std::tuple{packaged_environment_skybox_basis_asset_id(), kRuntimeEnvironmentSkyboxBasisPath,
                   std::string_view{"ktx2_basis"}},
    };

    for (const auto& [asset, path, source_kind] : expected_records) {
        const auto* record = runtime_package->find(asset);
        if (record == nullptr) {
            evidence.status = EnvironmentTextureAssetPipelinePackageStatus::missing_texture_record;
            evidence.diagnostics = 1U;
            return evidence;
        }
        evidence.package_index_entries += 1U;
        if (!is_environment_texture_metadata_record(*record, path, source_kind)) {
            evidence.status = EnvironmentTextureAssetPipelinePackageStatus::invalid_texture_record;
            evidence.diagnostics = 1U;
            return evidence;
        }

        const std::string_view content{record->content};
        evidence.metadata_records += 1U;
        evidence.metadata_only_records +=
            content.find("asset.payload=metadata_only\n") != std::string_view::npos ? 1U : 0U;
        evidence.openexr_records += source_kind == "openexr" ? 1U : 0U;
        evidence.ktx2_basis_records += source_kind == "ktx2_basis" ? 1U : 0U;
        evidence.source_hash_rows += content.find("source.hash=sha256:") != std::string_view::npos ? 1U : 0U;
        evidence.provenance_rows +=
            content.find("source.provenance_id=provenance.environment.") != std::string_view::npos ? 1U : 0U;
        evidence.license_rows +=
            content.find("source.license_id=LicenseRef-Proprietary\n") != std::string_view::npos ? 1U : 0U;
        evidence.backend_policy_rows += metadata_backend_policy_rows(content);
        evidence.unsupported_host_diagnostics += metadata_unsupported_host_diagnostics(content);
        evidence.environment_profile_dependency_refs += has_record_dependency(*environment_record, asset) ? 1U : 0U;
        evidence.environment_texture_dependency_edges +=
            has_environment_texture_dependency_edge(*runtime_package, asset) ? 1U : 0U;
    }

    if (evidence.environment_profile_dependency_refs != expected_records.size()) {
        evidence.status = EnvironmentTextureAssetPipelinePackageStatus::missing_profile_dependency;
        evidence.diagnostics = 1U;
        return evidence;
    }
    if (evidence.environment_texture_dependency_edges != expected_records.size()) {
        evidence.status = EnvironmentTextureAssetPipelinePackageStatus::missing_dependency_edge;
        evidence.diagnostics = 1U;
        return evidence;
    }

    evidence.status = EnvironmentTextureAssetPipelinePackageStatus::ready;
    evidence.ready = evidence.package_index_entries == 2U && evidence.metadata_records == 2U &&
                     evidence.metadata_only_records == 2U && evidence.openexr_records == 1U &&
                     evidence.ktx2_basis_records == 1U && evidence.source_hash_rows == 2U &&
                     evidence.provenance_rows == 2U && evidence.license_rows == 2U &&
                     evidence.backend_policy_rows == 10U && evidence.unsupported_host_diagnostics == 8U &&
                     !evidence.pixel_decode_invoked && !evidence.basis_runtime_transcode_invoked &&
                     !evidence.gpu_upload_invoked && !evidence.broad_asset_pipeline_ready;
    if (!evidence.ready) {
        evidence.status = EnvironmentTextureAssetPipelinePackageStatus::invalid_texture_record;
        evidence.diagnostics = 1U;
    }
    return evidence;
}

enum class EnvironmentLightingPackageStatus : std::uint8_t {
    not_requested,
    missing_package,
    missing_package_record,
    invalid_package_record,
    policy_failed,
    ready,
};

struct EnvironmentLightingPackageEvidence {
    EnvironmentLightingPackageStatus status{EnvironmentLightingPackageStatus::not_requested};
    bool requested{false};
    bool ready{false};
    bool package_file_present{false};
    bool package_index_entry_present{false};
    bool package_record_ready{false};
    bool package_source_ready{false};
    bool hdr_metadata_ready{false};
    bool package_evidence_ready{false};
    bool renderer_upload_evidence_ready{false};
    bool renderer_execution_requested{false};
    bool runtime_capture_evidence_ready{false};
    bool exposes_native_handles{false};
    std::uint32_t reflection_cubemap_rows{0};
    std::uint32_t reflection_cubemap_face_count{0};
    std::uint32_t reflection_cubemap_edge_size{0};
    std::uint32_t reflection_cubemap_mip_count{0};
    mirakana::EnvironmentLightingTextureFormat reflection_cubemap_format{
        mirakana::EnvironmentLightingTextureFormat::unknown};
    std::uint32_t irradiance_rows{0};
    std::uint32_t radiance_mip_rows{0};
    std::uint32_t package_evidence_rows{0};
    bool hdr_clamp_enabled{false};
    float hdr_clamp_max_luminance_nits{0.0F};
    std::uint32_t texture_uploads{0};
    std::uint32_t texture_cube_uploads{0};
    std::uint32_t texture_cube_faces{0};
    std::uint32_t texture_cube_edge_size{0};
    std::uint32_t renderer_radiance_mips{0};
    std::uint32_t renderer_irradiance_rows{0};
    bool shader_sampling_proven{false};
    bool shader_sample_readback_nonzero{false};
    std::uint32_t backend_invocations{0};
    std::uint32_t runtime_captures{0};
    std::uint32_t runtime_capture_faces{0};
    bool runtime_capture_readback_nonzero{false};
    std::uint64_t runtime_capture_readback_checksum{0};
    std::uint32_t diagnostics{0};
};

enum class EnvironmentAudioPlaybackStatus : std::uint8_t {
    not_requested,
    blocked,
    host_gated,
    ready,
};

struct EnvironmentAudioPlaybackEvidence {
    EnvironmentAudioPlaybackStatus status{EnvironmentAudioPlaybackStatus::not_requested};
    bool requested{false};
    bool ready{false};
    bool runtime_audio_lane_ready{false};
    bool device_host_evidence_available{false};
    bool device_io_invoked{false};
    std::uint32_t trigger_rows{0};
    std::uint32_t runtime_cues_started{0};
    std::uint32_t runtime_cues_stopped{0};
    std::uint32_t runtime_render_commands{0};
    std::uint32_t runtime_render_frames{0};
    std::uint32_t diagnostics{0};
    bool device_owned_by_environment{false};
    bool native_handle_access{false};
};

enum class EnvironmentMaterialWeatheringStatus : std::uint8_t {
    not_requested,
    missing_package,
    missing_material_record,
    invalid_material_record,
    policy_failed,
    ready,
};

struct EnvironmentMaterialWeatheringEvidence {
    EnvironmentMaterialWeatheringStatus status{EnvironmentMaterialWeatheringStatus::not_requested};
    bool requested{false};
    bool ready{false};
    bool package_record_ready{false};
    bool package_evidence_ready{false};
    bool shader_contract_evidence_ready{false};
    bool execution_evidence_ready{false};
    mirakana::EnvironmentMaterialWeatheringState state{mirakana::EnvironmentMaterialWeatheringState::dry};
    std::uint32_t constant_rows{0};
    std::uint32_t wet_rows{0};
    std::uint32_t snow_rows{0};
    std::uint32_t ice_rows{0};
    std::uint32_t quality_rows{0};
    std::uint32_t constants_binding{0};
    std::uint64_t constant_layout_bytes{0};
    std::uint64_t material_parameter_bindings{0};
    std::uint64_t material_constant_bytes{0};
    std::uint64_t backend_invocations{0};
    bool source_material_mutations{false};
    bool native_handle_access{false};
    std::uint32_t diagnostics{0};
};

enum class EnvironmentQualityBudgetStatus : std::uint8_t {
    not_requested,
    blocked,
    ready,
};

enum class EnvironmentReadyAggregateStatus : std::uint8_t {
    not_requested,
    blocked,
    ready,
};

struct EnvironmentQualityBudgetEvidence {
    EnvironmentQualityBudgetStatus status{EnvironmentQualityBudgetStatus::not_requested};
    bool requested{false};
    bool ready{false};
    std::string quality_preset{"unknown"};
    std::uint32_t feature_rows{0};
    std::uint32_t diagnostics{0};
    std::uint32_t violations{0};
    std::uint64_t constant_buffer_bytes{0};
    std::uint32_t physical_sky_sample_budget{0};
    std::uint32_t height_fog_sample_step_budget{0};
    std::uint32_t volumetric_fog_raymarch_step_budget{0};
    std::uint32_t volumetric_cloud_primary_step_budget{0};
    std::uint32_t volumetric_cloud_light_step_budget{0};
    std::uint32_t particle_rows{0};
    std::uint64_t texture_upload_budget{0};
    std::uint64_t particle_buffer_upload_budget{0};
    std::uint64_t renderer_draw_budget{0};
    std::uint64_t compute_dispatch_budget{0};
    std::uint64_t raymarch_pass_budget{0};
    std::uint32_t ibl_reflection_face_budget{0};
    std::uint32_t ibl_radiance_mip_budget{0};
    std::uint64_t transient_gpu_byte_estimate{0};
    std::uint64_t transient_heap_allocations{0};
    std::uint64_t transient_placed_allocations{0};
    std::uint64_t transient_placed_resources_alive{0};
    std::uint64_t framegraph_barrier_step_budget{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    bool native_handle_access{false};
    bool broad_optimization_claimed{false};
    bool broad_environment_ready_claimed{false};
};

struct EnvironmentReadyAggregateEvidence {
    EnvironmentReadyAggregateStatus status{EnvironmentReadyAggregateStatus::not_requested};
    bool requested{false};
    bool ready{false};
    bool profile_v2_ready{false};
    bool d3d12_primary_ready{false};
    bool vulkan_strict_ready{false};
    bool metal_host_ready{false};
    bool backend_parity_ready{false};
    bool broad_optimization_claimed{false};
    bool native_handle_access{false};
    std::uint32_t diagnostics{0};
};

struct EnvironmentVulkanStrictAggregateEvidence {
    EnvironmentReadyAggregateStatus status{EnvironmentReadyAggregateStatus::not_requested};
    bool requested{false};
    bool ready{false};
    bool profile_v2_ready{false};
    bool vulkan_backend_selected{false};
    bool postprocess_ready{false};
    bool fog_ready{false};
    bool physical_sky_ready{false};
    bool lighting_ready{false};
    bool volumetric_fog_ready{false};
    bool volumetric_cloud_ready{false};
    bool precipitation_ready{false};
    bool quality_budget_ready{false};
    std::uint32_t feature_rows{0};
    std::uint32_t descriptor_set_bindings{0};
    std::uint32_t synchronization2_barriers{0};
    bool toolchain_ready{false};
    bool vulkan_sdk_tools_ready{false};
    bool dxc_spirv_codegen_ready{false};
    bool spirv_validation_ready{false};
    bool validation_layers_ready{false};
    bool device_features_ready{false};
    std::uint32_t toolchain_rows{0};
    std::uint32_t missing_toolchain_rows{0};
    std::uint32_t missing_validation_layer_rows{0};
    std::uint32_t missing_spirv_validation_rows{0};
    std::uint32_t unsupported_feature_device_rows{0};
    bool resource_usage_layout_ready{false};
    std::uint32_t resource_usage_layout_rows{0};
    std::uint32_t attachment_usage_layout_rows{0};
    std::uint32_t sampled_texture_usage_layout_rows{0};
    std::uint32_t storage_buffer_usage_layout_rows{0};
    std::uint32_t cube_map_usage_layout_rows{0};
    std::uint32_t weather_texture_usage_layout_rows{0};
    std::uint32_t froxel_buffer_usage_layout_rows{0};
    std::uint32_t readback_resource_usage_layout_rows{0};
    std::uint64_t renderer_draws{0};
    std::uint64_t compute_dispatches{0};
    std::uint64_t texture_uploads{0};
    std::uint32_t readback_rows{0};
    bool native_handle_access{false};
    bool d3d12_fallback{false};
    bool metal_fallback{false};
    bool backend_parity_ready{false};
    bool broad_optimization_claimed{false};
    std::uint32_t diagnostics{0};
};

struct EnvironmentBackendParitySmokeEvidence {
    bool requested{false};
    mirakana::EnvironmentBackendParityPlan plan{};
};

struct EnvironmentPlatformReadinessSmokeEvidence {
    bool requested{false};
    bool ready{false};
    std::uint32_t rows{0U};
    std::uint32_t ready_rows{0U};
    std::uint32_t host_gated_rows{0U};
    bool windows_d3d12_ready{false};
    bool windows_vulkan_ready{false};
    bool linux_vulkan_ready{false};
    bool macos_metal_ready{false};
    bool ios_metal_ready{false};
    bool android_vulkan_ready{false};
    bool requires_windows_vulkan_host_evidence{true};
    bool requires_linux_vulkan_host_evidence{true};
    bool requires_macos_metal_host_evidence{true};
    bool requires_ios_metal_host_evidence{true};
    bool requires_android_vulkan_host_evidence{true};
    bool backend_parity_ready{false};
    bool commercial_ready{false};
    bool broad_environment_ready{false};
    bool broad_optimization_ready{false};
    bool native_handle_access{false};
    bool invoked_gpu_commands{false};
    std::uint32_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
};

struct EnvironmentOptimizationMeasurementSmokeEvidence {
    bool requested{false};
    mirakana::EnvironmentOptimizationMeasurementPlan plan{};
};

struct EnvironmentWeatherSimulationPackageEvidence {
    bool requested{false};
    mirakana::EnvironmentWeatherSimulationPlan plan{};
    std::uint32_t step_count{0U};
};

[[nodiscard]] std::string_view
environment_lighting_package_status_name(EnvironmentLightingPackageStatus status) noexcept {
    switch (status) {
    case EnvironmentLightingPackageStatus::not_requested:
        return "not_requested";
    case EnvironmentLightingPackageStatus::missing_package:
        return "missing_package";
    case EnvironmentLightingPackageStatus::missing_package_record:
        return "missing_package_record";
    case EnvironmentLightingPackageStatus::invalid_package_record:
        return "invalid_package_record";
    case EnvironmentLightingPackageStatus::policy_failed:
        return "policy_failed";
    case EnvironmentLightingPackageStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
environment_lighting_renderer_upload_status_name(const EnvironmentLightingPackageEvidence& evidence) noexcept {
    if (!evidence.renderer_execution_requested) {
        return "not_requested";
    }
    return evidence.renderer_upload_evidence_ready ? "ready" : "blocked";
}

[[nodiscard]] std::string_view environment_audio_playback_status_name(EnvironmentAudioPlaybackStatus status) noexcept {
    switch (status) {
    case EnvironmentAudioPlaybackStatus::not_requested:
        return "not_requested";
    case EnvironmentAudioPlaybackStatus::blocked:
        return "blocked";
    case EnvironmentAudioPlaybackStatus::host_gated:
        return "host_gated";
    case EnvironmentAudioPlaybackStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
environment_material_weathering_status_name(EnvironmentMaterialWeatheringStatus status) noexcept {
    switch (status) {
    case EnvironmentMaterialWeatheringStatus::not_requested:
        return "not_requested";
    case EnvironmentMaterialWeatheringStatus::missing_package:
        return "missing_package";
    case EnvironmentMaterialWeatheringStatus::missing_material_record:
        return "missing_material_record";
    case EnvironmentMaterialWeatheringStatus::invalid_material_record:
        return "invalid_material_record";
    case EnvironmentMaterialWeatheringStatus::policy_failed:
        return "policy_failed";
    case EnvironmentMaterialWeatheringStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
environment_material_weathering_state_name(mirakana::EnvironmentMaterialWeatheringState state) noexcept {
    switch (state) {
    case mirakana::EnvironmentMaterialWeatheringState::dry:
        return "dry";
    case mirakana::EnvironmentMaterialWeatheringState::wet:
        return "wet";
    case mirakana::EnvironmentMaterialWeatheringState::snow_covered:
        return "snow_covered";
    case mirakana::EnvironmentMaterialWeatheringState::icy:
        return "icy";
    case mirakana::EnvironmentMaterialWeatheringState::mixed:
        return "mixed";
    }
    return "unknown";
}

[[nodiscard]] std::string_view environment_quality_budget_status_name(EnvironmentQualityBudgetStatus status) noexcept {
    switch (status) {
    case EnvironmentQualityBudgetStatus::not_requested:
        return "not_requested";
    case EnvironmentQualityBudgetStatus::blocked:
        return "blocked";
    case EnvironmentQualityBudgetStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
environment_ready_aggregate_status_name(EnvironmentReadyAggregateStatus status) noexcept {
    switch (status) {
    case EnvironmentReadyAggregateStatus::not_requested:
        return "not_requested";
    case EnvironmentReadyAggregateStatus::blocked:
        return "blocked";
    case EnvironmentReadyAggregateStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
environment_backend_parity_status_name(mirakana::EnvironmentBackendParityStatus status) noexcept {
    switch (status) {
    case mirakana::EnvironmentBackendParityStatus::ready:
        return "ready";
    case mirakana::EnvironmentBackendParityStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::EnvironmentBackendParityStatus::no_rows:
        return "no_rows";
    case mirakana::EnvironmentBackendParityStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
environment_optimization_measurement_status_name(mirakana::EnvironmentOptimizationMeasurementStatus status) noexcept {
    switch (status) {
    case mirakana::EnvironmentOptimizationMeasurementStatus::ready:
        return "ready";
    case mirakana::EnvironmentOptimizationMeasurementStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::EnvironmentOptimizationMeasurementStatus::no_rows:
        return "no_rows";
    case mirakana::EnvironmentOptimizationMeasurementStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
environment_weather_simulation_package_status_name(const mirakana::EnvironmentWeatherSimulationPlan& plan) noexcept {
    return plan.succeeded() && plan.status == mirakana::EnvironmentWeatherSimulationStatus::stepped ? "ready"
                                                                                                    : "blocked";
}

[[nodiscard]] std::uint64_t kilograms_to_milligrams(double kilograms) noexcept {
    if (!std::isfinite(kilograms) || kilograms <= 0.0) {
        return 0ULL;
    }
    return static_cast<std::uint64_t>(std::llround(kilograms * 1000000.0));
}

[[nodiscard]] std::uint64_t seconds_to_milliseconds(float seconds) noexcept {
    if (!std::isfinite(seconds) || seconds <= 0.0F) {
        return 0ULL;
    }
    return static_cast<std::uint64_t>(std::llround(static_cast<double>(seconds) * 1000.0));
}

[[nodiscard]] constexpr std::uint64_t environment_weather_simulation_package_water_error_bound_mg() noexcept {
    return 1ULL;
}

[[nodiscard]] bool environment_quality_budget_requested(const DesktopRuntimeGameOptions& options) noexcept {
    return options.require_environment_profile || options.require_environment_fog_evidence ||
           options.require_environment_fog_vulkan_package_evidence || options.require_physical_sky_package_evidence ||
           options.require_physical_sky_vulkan_package_evidence ||
           options.require_environment_volumetric_fog_package_evidence ||
           options.require_environment_volumetric_fog_vulkan_renderer_execution ||
           options.require_environment_lighting_package_evidence ||
           options.require_environment_lighting_renderer_execution ||
           options.require_environment_lighting_vulkan_renderer_execution ||
           options.require_cloud_layer_package_evidence || options.require_cloud_layer_renderer_execution ||
           options.require_environment_precipitation_package_evidence ||
           options.require_environment_precipitation_renderer_execution ||
           options.require_environment_precipitation_vulkan_renderer_execution ||
           options.require_environment_snow_package_evidence || options.require_environment_snow_renderer_execution ||
           options.require_environment_volumetric_cloud_package_evidence ||
           options.require_environment_volumetric_cloud_renderer_execution ||
           options.require_environment_volumetric_cloud_vulkan_renderer_execution ||
           options.require_environment_material_weathering || options.require_environment_audio_playback ||
           options.require_environment_ready_aggregate || options.require_environment_backend_parity ||
           options.require_environment_platform_readiness || options.require_environment_optimization_measurement;
}

[[nodiscard]] std::uint32_t total_physical_sky_sample_budget(mirakana::PhysicalSkySampleBudgetDesc budget) noexcept {
    return budget.transmittance_sample_count + budget.sky_view_sample_count + budget.aerial_perspective_sample_count +
           budget.multiple_scattering_sample_count;
}

[[nodiscard]] std::uint64_t
transient_gpu_byte_estimate(const mirakana::Win32DesktopPresentationReport& report) noexcept {
    if (report.rhi_transient_heap_allocations > 0) {
        return report.rhi_transient_heap_allocations * 1024ULL * 1024ULL;
    }
    if (report.rhi_transient_placed_allocations > 0) {
        return report.rhi_transient_placed_allocations * 1024ULL * 1024ULL;
    }
    return 0;
}

[[nodiscard]] std::string_view
environment_lighting_texture_format_name(mirakana::EnvironmentLightingTextureFormat format) noexcept {
    switch (format) {
    case mirakana::EnvironmentLightingTextureFormat::rgba16_float:
        return "rgba16_float";
    case mirakana::EnvironmentLightingTextureFormat::rgba32_float:
        return "rgba32_float";
    case mirakana::EnvironmentLightingTextureFormat::rgbe9995:
        return "rgbe9995";
    case mirakana::EnvironmentLightingTextureFormat::rgba8_unorm:
        return "rgba8_unorm";
    case mirakana::EnvironmentLightingTextureFormat::unknown:
        return "unknown";
    }
    return "unknown";
}

[[nodiscard]] bool contains_payload_row(const mirakana::runtime::RuntimeAssetRecord& record,
                                        std::string_view row) noexcept {
    return record.content.find(row) != std::string::npos;
}

[[nodiscard]] bool is_cooked_environment_ibl_record(const mirakana::runtime::RuntimeAssetRecord& record) noexcept {
    return record.kind == mirakana::AssetKind::texture && record.path == kRuntimeEnvironmentIblCubemapPath &&
           std::string_view{record.content}.starts_with("format=GameEngine.CookedTexture.v1\n") &&
           contains_payload_row(record, "asset.kind=texture\n") &&
           contains_payload_row(record, "texture.ibl.role=reflection_cubemap\n") &&
           contains_payload_row(record, "texture.ibl.source_format=GameEngine.FirstPartyIblMetadata.v1\n") &&
           contains_payload_row(record, "texture.ibl.format=rgba16_float\n") &&
           contains_payload_row(record, "texture.ibl.face_count=6\n") &&
           contains_payload_row(record, "texture.ibl.edge_size=16\n") &&
           contains_payload_row(record, "texture.ibl.mip_count=5\n") &&
           contains_payload_row(record, "texture.ibl.irradiance_order=3\n") &&
           contains_payload_row(record, "texture.ibl.irradiance_coefficients=9\n") &&
           contains_payload_row(record, "texture.ibl.radiance_mip_count=5\n") &&
           contains_payload_row(record, "texture.ibl.hdr_metadata=ready\n") &&
           contains_payload_row(record, "texture.ibl.hdr_clamp_nits=20000\n");
}

[[nodiscard]] EnvironmentLightingPackageEvidence evaluate_environment_lighting_package(
    bool requested, bool renderer_execution_requested,
    const mirakana::Win32DesktopPresentationEnvironmentIblRendererExecutionReport& renderer_execution,
    const std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package) {
    EnvironmentLightingPackageEvidence evidence;
    evidence.requested = requested;
    evidence.renderer_execution_requested = renderer_execution_requested;
    if (!requested) {
        return evidence;
    }
    if (!runtime_package.has_value()) {
        evidence.status = EnvironmentLightingPackageStatus::missing_package;
        evidence.diagnostics = 1;
        return evidence;
    }

    const auto ibl_asset = packaged_environment_ibl_cubemap_asset_id();
    const auto* ibl_record = runtime_package->find(ibl_asset);
    evidence.package_index_entry_present = ibl_record != nullptr;
    if (ibl_record == nullptr) {
        evidence.status = EnvironmentLightingPackageStatus::missing_package_record;
        evidence.diagnostics = 1;
        return evidence;
    }

    evidence.package_file_present = is_cooked_environment_ibl_record(*ibl_record);
    evidence.package_record_ready = evidence.package_index_entry_present && evidence.package_file_present;
    evidence.package_source_ready =
        evidence.package_file_present &&
        contains_payload_row(*ibl_record, "texture.ibl.source_format=GameEngine.FirstPartyIblMetadata.v1\n");
    evidence.hdr_metadata_ready =
        evidence.package_file_present && contains_payload_row(*ibl_record, "texture.ibl.hdr_metadata=ready\n");
    if (!evidence.package_file_present) {
        evidence.status = EnvironmentLightingPackageStatus::invalid_package_record;
        evidence.diagnostics = 1;
        return evidence;
    }

    auto policy_desc = make_sample_environment_lighting_policy_desc(
        evidence.package_record_ready, evidence.package_source_ready, evidence.hdr_metadata_ready);
    if (renderer_execution_requested) {
        policy_desc.request_backend_execution = true;
        policy_desc.request_runtime_capture = true;
        policy_desc.renderer_upload = mirakana::EnvironmentLightingRendererUploadEvidenceDesc{
            .evidence_ready = renderer_execution.ready,
            .texture_cube_uploads = renderer_execution.texture_cube_uploads,
            .texture_cube_faces = renderer_execution.texture_cube_faces,
            .texture_cube_edge_size = renderer_execution.texture_cube_edge_size,
            .radiance_mips = renderer_execution.radiance_mips,
            .irradiance_rows = renderer_execution.irradiance_rows,
            .shader_sampling_proven = renderer_execution.shader_sampling_proven,
            .shader_sample_readback_nonzero = renderer_execution.shader_sample_readback_nonzero,
        };
        policy_desc.runtime_capture = mirakana::EnvironmentLightingRuntimeCaptureEvidenceDesc{
            .evidence_ready = renderer_execution.ready,
            .capture_faces = renderer_execution.runtime_capture_faces,
            .readback_nonzero = renderer_execution.runtime_capture_readback_nonzero,
            .readback_checksum = renderer_execution.runtime_capture_readback_checksum,
        };
    }

    const auto policy = mirakana::plan_environment_lighting_policy(policy_desc);
    evidence.diagnostics =
        std::max(static_cast<std::uint32_t>(policy.diagnostics.size()), renderer_execution.diagnostics_count);
    evidence.package_evidence_ready = !policy.package_evidence_rows.empty() && policy.package_evidence_rows[0].ready;
    evidence.renderer_upload_evidence_ready = policy.renderer_upload_evidence_ready;
    evidence.runtime_capture_evidence_ready = policy.runtime_capture_evidence_ready;
    evidence.exposes_native_handles = policy.exposes_native_handles;
    evidence.reflection_cubemap_rows = static_cast<std::uint32_t>(policy.reflection_cubemap_rows.size());
    if (!policy.reflection_cubemap_rows.empty()) {
        evidence.reflection_cubemap_face_count = policy.reflection_cubemap_rows[0].face_count;
        evidence.reflection_cubemap_edge_size = policy.reflection_cubemap_rows[0].edge_size;
        evidence.reflection_cubemap_mip_count = policy.reflection_cubemap_rows[0].mip_count;
        evidence.reflection_cubemap_format = policy.reflection_cubemap_rows[0].format;
    }
    evidence.irradiance_rows = static_cast<std::uint32_t>(policy.irradiance_rows.size());
    evidence.radiance_mip_rows = static_cast<std::uint32_t>(policy.radiance_mip_rows.size());
    evidence.package_evidence_rows = static_cast<std::uint32_t>(policy.package_evidence_rows.size());
    evidence.hdr_clamp_enabled = policy.hdr_clamp_row.enabled;
    evidence.hdr_clamp_max_luminance_nits = policy.hdr_clamp_row.max_luminance_nits;
    evidence.texture_uploads = policy.texture_uploads;
    evidence.texture_cube_uploads = policy.texture_cube_uploads;
    evidence.texture_cube_faces = policy.texture_cube_faces;
    evidence.texture_cube_edge_size = policy.texture_cube_edge_size;
    evidence.renderer_radiance_mips = policy.renderer_radiance_mips;
    evidence.renderer_irradiance_rows = policy.renderer_irradiance_rows;
    evidence.shader_sampling_proven = policy.shader_sampling_proven;
    evidence.shader_sample_readback_nonzero = policy.shader_sample_readback_nonzero;
    evidence.backend_invocations = policy.backend_invocations;
    evidence.runtime_captures = policy.runtime_captures;
    evidence.runtime_capture_faces = policy.runtime_capture_faces;
    evidence.runtime_capture_readback_nonzero = policy.runtime_capture_readback_nonzero;
    evidence.runtime_capture_readback_checksum = policy.runtime_capture_readback_checksum;

    if (!policy.succeeded()) {
        evidence.status = EnvironmentLightingPackageStatus::policy_failed;
        evidence.diagnostics = std::max(evidence.diagnostics, 1U);
        return evidence;
    }

    if (renderer_execution_requested) {
        evidence.ready = evidence.package_evidence_ready && evidence.renderer_upload_evidence_ready &&
                         evidence.runtime_capture_evidence_ready && !evidence.exposes_native_handles &&
                         evidence.texture_uploads > 0U && evidence.texture_cube_uploads > 0U &&
                         evidence.texture_cube_faces == 6U && evidence.texture_cube_edge_size == 16U &&
                         evidence.renderer_radiance_mips >= 5U && evidence.renderer_irradiance_rows == 9U &&
                         evidence.shader_sampling_proven && evidence.shader_sample_readback_nonzero &&
                         evidence.backend_invocations > 0U && evidence.runtime_captures > 0U &&
                         evidence.runtime_capture_faces == 6U && evidence.runtime_capture_readback_nonzero &&
                         evidence.runtime_capture_readback_checksum > 0U &&
                         renderer_execution.native_handle_access == 0U;
    } else {
        evidence.ready = evidence.package_evidence_ready && !evidence.renderer_upload_evidence_ready &&
                         !evidence.runtime_capture_evidence_ready && !evidence.exposes_native_handles &&
                         evidence.texture_uploads == 0U && evidence.texture_cube_uploads == 0U &&
                         evidence.backend_invocations == 0U && evidence.runtime_captures == 0U;
    }
    evidence.status =
        evidence.ready ? EnvironmentLightingPackageStatus::ready : EnvironmentLightingPackageStatus::policy_failed;
    evidence.diagnostics = evidence.ready ? 0U : std::max(evidence.diagnostics, 1U);
    return evidence;
}

[[nodiscard]] mirakana::EnvironmentWeatherAudioPlaybackPlan make_sample_environment_audio_playback_plan() {
    const auto precipitation = make_sample_environment_precipitation_policy_desc();
    return mirakana::plan_environment_weather_audio_playback(mirakana::EnvironmentWeatherAudioPlaybackDesc{
        .handoff_rows = precipitation.environment_plan.audio_handoff_rows,
        .cue_bindings = mirakana::default_environment_weather_audio_cue_bindings(),
    });
}

[[nodiscard]] mirakana::AudioClipSampleData make_environment_audio_clip_samples(mirakana::AssetId clip,
                                                                                std::uint32_t source_index) {
    constexpr auto frame_count = std::uint64_t{256U};
    constexpr auto channel_count = std::uint32_t{2U};
    std::vector<float> samples;
    samples.reserve(static_cast<std::size_t>(frame_count * channel_count));
    const auto base_sample = 0.03F + (static_cast<float>(source_index) * 0.01F);
    for (std::uint64_t frame = 0; frame < frame_count; ++frame) {
        const auto pulse = (frame % 17U) == 0U ? 0.025F : 0.0F;
        samples.push_back(base_sample + pulse);
        samples.push_back((base_sample * 0.75F) + pulse);
    }
    return mirakana::AudioClipSampleData{
        .clip = clip,
        .format =
            mirakana::AudioDeviceFormat{
                .sample_rate = 48000,
                .channel_count = channel_count,
                .sample_format = mirakana::AudioSampleFormat::float32,
            },
        .frame_count = frame_count,
        .interleaved_float_samples = std::move(samples),
    };
}

[[nodiscard]] EnvironmentAudioPlaybackEvidence evaluate_environment_audio_playback(bool requested) {
    EnvironmentAudioPlaybackEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    const auto audio_plan = make_sample_environment_audio_playback_plan();
    evidence.trigger_rows = static_cast<std::uint32_t>(audio_plan.trigger_rows.size());
    evidence.device_owned_by_environment = audio_plan.owns_audio_device;
    evidence.native_handle_access = audio_plan.exposes_native_handles;
    evidence.diagnostics = static_cast<std::uint32_t>(audio_plan.diagnostics.size());
    if (!audio_plan.succeeded()) {
        evidence.status = EnvironmentAudioPlaybackStatus::blocked;
        evidence.diagnostics = std::max(evidence.diagnostics, 1U);
        return evidence;
    }

    try {
        mirakana::AudioGameplayMixRequest request;
        request.buses.push_back(mirakana::AudioGameplayBusMixDesc{
            .name = "environment.weather",
            .gain = 1.0F,
            .muted = false,
            .paused = false,
            .fade_from_gain = 1.0F,
            .fade_to_gain = 1.0F,
            .fade_elapsed_seconds = 0.0F,
            .fade_duration_seconds = 0.0F,
        });

        mirakana::AudioMixer mixer;
        std::vector<mirakana::AudioClipSampleData> sample_rows;
        sample_rows.reserve(audio_plan.trigger_rows.size());
        mixer.add_bus(mirakana::AudioBusDesc{.name = "environment.weather", .gain = 1.0F, .muted = false});

        std::uint32_t source_index = 0U;
        for (const auto& row : audio_plan.trigger_rows) {
            const auto clip = asset_id_from_game_asset_key(row.clip_asset_ref);
            mixer.add_clip(mirakana::AudioClipDesc{
                .clip = clip,
                .sample_rate = 48000,
                .channel_count = 2,
                .frame_count = 256U,
                .sample_format = mirakana::AudioSampleFormat::float32,
                .streaming = false,
                .buffered_frame_count = 0U,
            });
            sample_rows.push_back(make_environment_audio_clip_samples(clip, source_index++));
            request.cues.push_back(mirakana::AudioGameplayCueDesc{
                .id = row.cue_id,
                .kind = row.one_shot ? mirakana::AudioGameplayCueKind::sfx : mirakana::AudioGameplayCueKind::ambience,
                .clip = clip,
                .bus = row.bus,
                .gain = row.gain,
                .looping = row.looping,
                .spatialized = false,
                .position = mirakana::AudioPoint3{},
                .min_distance = 1.0F,
                .max_distance = 64.0F,
            });
            request.triggers.push_back(mirakana::AudioGameplayCueTrigger{
                .cue_id = row.cue_id,
                .start_frame = static_cast<std::uint64_t>(row.delay_seconds * 48000.0F),
                .gain_scale = 1.0F,
            });
        }

        const auto mix = mirakana::plan_gameplay_audio_mix(request);
        evidence.diagnostics += static_cast<std::uint32_t>(mix.diagnostics.size());
        if (!mix.succeeded()) {
            evidence.status = EnvironmentAudioPlaybackStatus::blocked;
            evidence.diagnostics = std::max(evidence.diagnostics, 1U);
            return evidence;
        }

        for (const auto& bus : mix.buses) {
            mixer.set_bus_gain(bus.name, bus.gain);
            mixer.set_bus_muted(bus.name, bus.muted);
        }
        for (const auto& command : mix.commands) {
            (void)mixer.play(command.voice);
            ++evidence.runtime_cues_started;
            if (!command.voice.looping) {
                ++evidence.runtime_cues_stopped;
            }
        }

        const auto render = mixer.render_interleaved_float(
            mirakana::AudioRenderRequest{
                .format =
                    mirakana::AudioDeviceFormat{
                        .sample_rate = 48000,
                        .channel_count = 2,
                        .sample_format = mirakana::AudioSampleFormat::float32,
                    },
                .frame_count = 512U,
                .device_frame = 0U,
                .underrun_warning_threshold_frames = 0U,
                .resampling_quality = mirakana::AudioResamplingQuality::linear,
            },
            std::span<const mirakana::AudioClipSampleData>{sample_rows});
        evidence.runtime_render_commands = static_cast<std::uint32_t>(render.plan.commands.size());
        evidence.runtime_render_frames = render.frame_count;
        float sample_abs_sum = 0.0F;
        for (const auto sample : render.interleaved_float_samples) {
            sample_abs_sum += std::fabs(sample);
        }

        evidence.runtime_audio_lane_ready = evidence.trigger_rows == 4U && evidence.runtime_cues_started > 0U &&
                                            evidence.runtime_render_commands > 0U &&
                                            evidence.runtime_render_frames > 0U && sample_abs_sum > 0.0F;
        evidence.ready = evidence.runtime_audio_lane_ready && !evidence.device_owned_by_environment &&
                         !evidence.native_handle_access && !evidence.device_io_invoked && evidence.diagnostics == 0U;
        evidence.status =
            evidence.ready ? EnvironmentAudioPlaybackStatus::ready : EnvironmentAudioPlaybackStatus::host_gated;
    } catch (const std::exception&) {
        evidence.status = EnvironmentAudioPlaybackStatus::blocked;
        evidence.diagnostics = std::max(evidence.diagnostics + 1U, 1U);
    }
    return evidence;
}

[[nodiscard]] bool is_sample_material_record(const mirakana::runtime::RuntimeAssetRecord& record) noexcept {
    return record.kind == mirakana::AssetKind::material &&
           record.path == "runtime/assets/desktop_runtime/unlit.material" &&
           std::string_view{record.content}.starts_with("format=GameEngine.Material.v1\n") &&
           contains_payload_row(record, "material.shading=lit\n") &&
           contains_payload_row(record, "factor.roughness=1\n") &&
           contains_payload_row(record, "texture.1.slot=base_color\n");
}

[[nodiscard]] bool
material_record_has_weathering_source_mutation(const mirakana::runtime::RuntimeAssetRecord& record) noexcept {
    const auto content = std::string_view{record.content};
    return content.find("material.weathering") != std::string_view::npos ||
           content.find("environment_material_weathering") != std::string_view::npos ||
           content.find("weathering.source_material_mutation") != std::string_view::npos;
}

[[nodiscard]] EnvironmentMaterialWeatheringEvidence
evaluate_environment_material_weathering(bool requested, const mirakana::Win32DesktopPresentationReport& report,
                                         const std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package) {
    EnvironmentMaterialWeatheringEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }
    if (!runtime_package.has_value()) {
        evidence.status = EnvironmentMaterialWeatheringStatus::missing_package;
        evidence.diagnostics = 1;
        return evidence;
    }

    const auto* material_record = runtime_package->find(packaged_material_asset_id());
    if (material_record == nullptr) {
        evidence.status = EnvironmentMaterialWeatheringStatus::missing_material_record;
        evidence.diagnostics = 1;
        return evidence;
    }
    evidence.package_record_ready = is_sample_material_record(*material_record);
    evidence.source_material_mutations = material_record_has_weathering_source_mutation(*material_record);
    if (!evidence.package_record_ready || evidence.source_material_mutations) {
        evidence.status = EnvironmentMaterialWeatheringStatus::invalid_material_record;
        evidence.diagnostics = 1;
        return evidence;
    }

    const auto material_parameter_bindings = static_cast<std::uint64_t>(report.scene_gpu_stats.material_bindings);
    const auto material_constant_bytes =
        static_cast<std::uint64_t>(report.scene_gpu_stats.uploaded_material_factor_bytes);
    const auto backend_invocations = static_cast<std::uint64_t>(report.scene_gpu_stats.material_uploads);
    const auto policy =
        mirakana::plan_material_weathering_policy(make_sample_environment_material_weathering_policy_desc(
            report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12, material_parameter_bindings,
            material_constant_bytes, backend_invocations));

    evidence.shader_contract_evidence_ready = policy.shader_contract_evidence_ready;
    evidence.package_evidence_ready = policy.package_evidence_ready && evidence.package_record_ready;
    evidence.execution_evidence_ready = policy.execution_evidence_ready;
    evidence.material_parameter_bindings = policy.material_parameter_bindings;
    evidence.material_constant_bytes = policy.material_constant_bytes_uploaded;
    evidence.backend_invocations = policy.backend_invocations;
    evidence.native_handle_access = policy.exposes_native_handles;
    evidence.source_material_mutations = evidence.source_material_mutations || policy.mutates_source_materials;
    evidence.constant_rows = static_cast<std::uint32_t>(policy.constant_rows.size());
    evidence.wet_rows = static_cast<std::uint32_t>(policy.wet_rows.size());
    evidence.snow_rows = static_cast<std::uint32_t>(policy.snow_rows.size());
    evidence.ice_rows = static_cast<std::uint32_t>(policy.ice_rows.size());
    evidence.quality_rows = static_cast<std::uint32_t>(policy.quality_rows.size());
    if (!policy.constant_rows.empty()) {
        evidence.state = policy.constant_rows[0].state;
        evidence.constants_binding = policy.constant_rows[0].constants_binding_slot;
        evidence.constant_layout_bytes = policy.constant_rows[0].constant_bytes;
    }
    evidence.diagnostics = static_cast<std::uint32_t>(policy.diagnostics.size());
    evidence.ready = policy.ready() && evidence.package_evidence_ready && evidence.execution_evidence_ready &&
                     evidence.constant_rows == 1U && evidence.wet_rows > 0U && evidence.snow_rows > 0U &&
                     evidence.material_parameter_bindings > 0U && evidence.material_constant_bytes > 0U &&
                     evidence.backend_invocations > 0U && !evidence.native_handle_access &&
                     !evidence.source_material_mutations && evidence.diagnostics == 0U;
    evidence.status = evidence.ready ? EnvironmentMaterialWeatheringStatus::ready
                                     : EnvironmentMaterialWeatheringStatus::policy_failed;
    if (!evidence.ready) {
        evidence.diagnostics = std::max(evidence.diagnostics, 1U);
    }
    return evidence;
}

[[nodiscard]] EnvironmentQualityBudgetEvidence build_environment_quality_budget_evidence(
    const DesktopRuntimeGameOptions& options, const mirakana::Win32DesktopPresentationReport& report,
    const mirakana::Win32DesktopPresentationEnvironmentFogReport& environment_fog,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentFogPackageReport& environment_fog_vulkan_package,
    const mirakana::Win32DesktopPresentationPhysicalSkyReport& physical_sky,
    const mirakana::Win32DesktopPresentationVulkanPhysicalSkyPackageReport& physical_sky_vulkan_package,
    const EnvironmentLightingPackageEvidence& environment_lighting,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionReport&
        environment_ibl_vulkan_renderer_execution,
    const EnvironmentMaterialWeatheringEvidence& environment_material_weathering,
    const mirakana::Win32DesktopPresentationCloudLayerReport& cloud_layer,
    const mirakana::Win32DesktopPresentationEnvironmentPrecipitationReport& environment_precipitation,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentPrecipitationReport& environment_precipitation_vulkan,
    const mirakana::Win32DesktopPresentationEnvironmentVolumetricFogReport& environment_volumetric_fog,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentVolumetricFogReport& environment_volumetric_fog_vulkan,
    const mirakana::Win32DesktopPresentationEnvironmentVolumetricCloudReport& environment_volumetric_cloud,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentVolumetricCloudReport& environment_volumetric_cloud_vulkan,
    const EnvironmentProfilePackageEvidence& environment_profile,
    const EnvironmentAudioPlaybackEvidence& environment_audio_playback,
    std::uint64_t expected_framegraph_barrier_steps) {
    EnvironmentQualityBudgetEvidence evidence;
    evidence.requested = environment_quality_budget_requested(options);
    if (!evidence.requested) {
        return evidence;
    }
    evidence.quality_preset = environment_profile.quality_preset;

    auto add_feature = [&](const bool selected, const bool ready) {
        if (!selected) {
            return;
        }
        ++evidence.feature_rows;
        if (!ready) {
            ++evidence.diagnostics;
        }
    };

    const auto physical_sky_desc = make_sample_physical_sky_policy_desc();
    const auto fog_desc = make_sample_environment_fog_policy_desc();
    const auto volumetric_fog_desc = make_sample_environment_volumetric_fog_policy_desc(true);
    const auto volumetric_cloud_desc = make_sample_environment_volumetric_cloud_policy_desc();

    add_feature(options.require_environment_profile, environment_profile.ready);
    add_feature(options.require_physical_sky_package_evidence, physical_sky.ready);
    add_feature(options.require_physical_sky_vulkan_package_evidence, physical_sky_vulkan_package.ready);
    add_feature(options.require_environment_fog_evidence, environment_fog.ready);
    add_feature(options.require_environment_fog_vulkan_package_evidence, environment_fog_vulkan_package.ready);
    add_feature(options.require_environment_volumetric_fog_package_evidence ||
                    options.require_environment_volumetric_fog_vulkan_renderer_execution,
                options.require_environment_volumetric_fog_vulkan_renderer_execution
                    ? environment_volumetric_fog_vulkan.ready
                    : environment_volumetric_fog.ready);
    add_feature(options.require_environment_lighting_package_evidence, environment_lighting.ready);
    add_feature(options.require_environment_lighting_vulkan_renderer_execution,
                environment_ibl_vulkan_renderer_execution.ready);
    add_feature(options.require_environment_material_weathering, environment_material_weathering.ready);
    add_feature(options.require_environment_audio_playback, environment_audio_playback.ready);
    add_feature(options.require_cloud_layer_package_evidence || options.require_cloud_layer_renderer_execution,
                cloud_layer.ready);
    add_feature(
        options.require_environment_precipitation_package_evidence ||
            options.require_environment_precipitation_renderer_execution ||
            options.require_environment_precipitation_vulkan_renderer_execution ||
            options.require_environment_snow_package_evidence || options.require_environment_snow_renderer_execution,
        options.require_environment_precipitation_vulkan_renderer_execution ? environment_precipitation_vulkan.ready
                                                                            : environment_precipitation.ready);
    add_feature(options.require_environment_volumetric_cloud_package_evidence ||
                    options.require_environment_volumetric_cloud_renderer_execution ||
                    options.require_environment_volumetric_cloud_vulkan_renderer_execution,
                options.require_environment_volumetric_cloud_vulkan_renderer_execution
                    ? environment_volumetric_cloud_vulkan.ready
                    : environment_volumetric_cloud.ready);

    if (options.require_physical_sky_package_evidence) {
        evidence.constant_buffer_bytes += physical_sky.constant_buffer_bytes;
        evidence.physical_sky_sample_budget = total_physical_sky_sample_budget(physical_sky_desc.sample_budget);
    }
    if (options.require_physical_sky_vulkan_package_evidence) {
        evidence.constant_buffer_bytes += physical_sky_vulkan_package.constant_buffer_bytes;
        evidence.physical_sky_sample_budget = total_physical_sky_sample_budget(physical_sky_desc.sample_budget);
    }
    if (options.require_environment_fog_evidence) {
        evidence.constant_buffer_bytes += environment_fog.constant_buffer_bytes;
        evidence.height_fog_sample_step_budget = fog_desc.sample_step_budget;
    }
    if (options.require_environment_fog_vulkan_package_evidence) {
        evidence.constant_buffer_bytes += environment_fog_vulkan_package.constant_buffer_bytes;
        evidence.height_fog_sample_step_budget = fog_desc.sample_step_budget;
    }
    if (options.require_environment_volumetric_fog_package_evidence) {
        evidence.constant_buffer_bytes += environment_volumetric_fog.constant_buffer_bytes;
        evidence.volumetric_fog_raymarch_step_budget = volumetric_fog_desc.raymarch_step_budget;
        evidence.compute_dispatch_budget = 1;
    }
    if (options.require_environment_volumetric_fog_vulkan_renderer_execution) {
        evidence.constant_buffer_bytes += environment_volumetric_fog_vulkan.constant_buffer_bytes;
        evidence.volumetric_fog_raymarch_step_budget = volumetric_fog_desc.raymarch_step_budget;
        evidence.compute_dispatch_budget = static_cast<std::uint32_t>(std::min<std::uint64_t>(
            environment_volumetric_fog_vulkan.compute_dispatches, std::numeric_limits<std::uint32_t>::max()));
    }
    if (options.require_cloud_layer_package_evidence || options.require_cloud_layer_renderer_execution) {
        evidence.constant_buffer_bytes += mirakana::cloud_layer_constants_byte_size();
        evidence.texture_upload_budget += options.require_cloud_layer_renderer_execution ? 1U : 0U;
        evidence.renderer_draw_budget += options.require_cloud_layer_renderer_execution ? 1U : 0U;
    }
    if (options.require_environment_precipitation_package_evidence ||
        options.require_environment_precipitation_renderer_execution ||
        options.require_environment_precipitation_vulkan_renderer_execution ||
        options.require_environment_snow_package_evidence || options.require_environment_snow_renderer_execution) {
        evidence.constant_buffer_bytes += mirakana::precipitation_constants_byte_size();
        evidence.particle_rows += options.require_environment_precipitation_vulkan_renderer_execution
                                      ? environment_precipitation_vulkan.particle_rows
                                      : environment_precipitation.particle_rows;
        evidence.particle_buffer_upload_budget +=
            (options.require_environment_precipitation_renderer_execution ||
             options.require_environment_precipitation_vulkan_renderer_execution ||
             options.require_environment_snow_renderer_execution)
                ? 1U
                : 0U;
        evidence.renderer_draw_budget += (options.require_environment_precipitation_renderer_execution ||
                                          options.require_environment_precipitation_vulkan_renderer_execution ||
                                          options.require_environment_snow_renderer_execution)
                                             ? 1U
                                             : 0U;
    }
    if (options.require_environment_volumetric_cloud_package_evidence ||
        options.require_environment_volumetric_cloud_renderer_execution ||
        options.require_environment_volumetric_cloud_vulkan_renderer_execution) {
        evidence.constant_buffer_bytes += options.require_environment_volumetric_cloud_vulkan_renderer_execution
                                              ? environment_volumetric_cloud_vulkan.constant_buffer_bytes
                                              : environment_volumetric_cloud.constant_buffer_bytes;
        evidence.volumetric_cloud_primary_step_budget = volumetric_cloud_desc.raymarch.primary_steps;
        evidence.volumetric_cloud_light_step_budget = volumetric_cloud_desc.raymarch.light_steps;
        const bool require_cloud_renderer_execution =
            options.require_environment_volumetric_cloud_renderer_execution ||
            options.require_environment_volumetric_cloud_vulkan_renderer_execution;
        evidence.texture_upload_budget += require_cloud_renderer_execution ? 3U : 0U;
        evidence.renderer_draw_budget += require_cloud_renderer_execution ? 1U : 0U;
        evidence.raymarch_pass_budget += require_cloud_renderer_execution ? 1U : 0U;
    }
    if (options.require_environment_lighting_package_evidence) {
        evidence.ibl_reflection_face_budget = environment_lighting.reflection_cubemap_face_count;
        evidence.ibl_radiance_mip_budget = environment_lighting.radiance_mip_rows;
    }
    if (options.require_environment_lighting_vulkan_renderer_execution) {
        evidence.ibl_reflection_face_budget = environment_ibl_vulkan_renderer_execution.texture_cube_faces;
        evidence.ibl_radiance_mip_budget = environment_ibl_vulkan_renderer_execution.radiance_mips;
        evidence.texture_upload_budget += environment_ibl_vulkan_renderer_execution.texture_cube_uploads > 0U ? 1U : 0U;
    }
    if (options.require_environment_material_weathering) {
        evidence.constant_buffer_bytes += mirakana::material_weathering_constants_byte_size();
    }

    evidence.transient_gpu_byte_estimate = transient_gpu_byte_estimate(report);
    evidence.transient_heap_allocations = report.rhi_transient_heap_allocations;
    evidence.transient_placed_allocations = report.rhi_transient_placed_allocations;
    evidence.transient_placed_resources_alive = report.rhi_transient_placed_resources_alive;
    evidence.framegraph_barrier_step_budget = expected_framegraph_barrier_steps;
    evidence.framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed;
    evidence.native_handle_access =
        physical_sky.exposes_native_handles || physical_sky_vulkan_package.exposes_native_handles ||
        environment_fog_vulkan_package.exposes_native_handles || environment_lighting.exposes_native_handles ||
        environment_ibl_vulkan_renderer_execution.native_handle_access != 0U || cloud_layer.exposes_native_handles ||
        environment_precipitation.exposes_native_handles || environment_precipitation_vulkan.exposes_native_handles ||
        environment_volumetric_fog.exposes_native_handles || environment_volumetric_cloud.exposes_native_handles ||
        environment_volumetric_cloud_vulkan.exposes_native_handles ||
        environment_material_weathering.native_handle_access || environment_audio_playback.native_handle_access;

    if (evidence.feature_rows == 0) {
        ++evidence.diagnostics;
    }
    if (evidence.native_handle_access || evidence.broad_environment_ready_claimed ||
        evidence.transient_placed_resources_alive != 0U) {
        ++evidence.diagnostics;
    }

    mirakana::EnvironmentQualityPreset selected_preset{mirakana::EnvironmentQualityPreset::medium};
    try {
        selected_preset = mirakana::parse_environment_quality_preset(evidence.quality_preset);
    } catch (const std::invalid_argument&) {
        ++evidence.diagnostics;
    }
    const auto budget_plan =
        mirakana::evaluate_environment_quality_budget(
            mirakana::EnvironmentQualityBudgetRequest{
                .preset = selected_preset,
                .usage =
                    mirakana::EnvironmentQualityBudgetUsageDesc{
                        .physical_sky_sample_budget = evidence.physical_sky_sample_budget,
                        .height_fog_sample_step_budget = evidence.height_fog_sample_step_budget,
                        .volumetric_fog_raymarch_step_budget = evidence.volumetric_fog_raymarch_step_budget,
                        .volumetric_cloud_primary_step_budget = evidence.volumetric_cloud_primary_step_budget,
                        .volumetric_cloud_light_step_budget = evidence.volumetric_cloud_light_step_budget,
                        .precipitation_particle_rows = evidence.particle_rows,
                        .transient_gpu_byte_estimate = evidence.transient_gpu_byte_estimate,
                        .framegraph_passes = static_cast<std::uint32_t>(
                            std::min<std::uint64_t>(report.renderer_stats.framegraph_passes_executed,
                                                    std::numeric_limits<std::uint32_t>::max())),
                        .framegraph_barrier_steps = static_cast<std::uint32_t>(
                            std::min<std::uint64_t>(std::max(evidence.framegraph_barrier_step_budget,
                                                             evidence.framegraph_barrier_steps_executed),
                                                    std::numeric_limits<std::uint32_t>::max())),
                        .texture_uploads =
                            static_cast<std::uint32_t>(
                                std::min<std::uint64_t>(evidence.texture_upload_budget, std::numeric_limits<
                                                                                            std::uint32_t>::max())),
                        .renderer_draws =
                            static_cast<std::uint32_t>(
                                std::min<std::uint64_t>(evidence.renderer_draw_budget,
                                                        std::numeric_limits<std::uint32_t>::max())),
                        .compute_dispatches =
                            static_cast<std::uint32_t>(
                                std::min<std::uint64_t>(evidence.compute_dispatch_budget,
                                                        std::numeric_limits<std::uint32_t>::max())),
                        .diagnostics = evidence.diagnostics,
                        .native_handle_access = evidence.native_handle_access,
                        .broad_optimization_claimed = evidence.broad_optimization_claimed,
                    },
            });
    evidence.violations = static_cast<std::uint32_t>(
        std::min<std::size_t>(budget_plan.diagnostics.size(), std::numeric_limits<std::uint32_t>::max()));
    evidence.diagnostics += evidence.violations;
    evidence.ready = evidence.diagnostics == 0U && budget_plan.ready;
    evidence.status = evidence.ready ? EnvironmentQualityBudgetStatus::ready : EnvironmentQualityBudgetStatus::blocked;
    return evidence;
}

[[nodiscard]] EnvironmentReadyAggregateEvidence build_environment_ready_aggregate_evidence(
    const DesktopRuntimeGameOptions& options, const mirakana::Win32DesktopPresentationReport& report,
    const mirakana::Win32DesktopPresentationEnvironmentFogReport& environment_fog,
    const mirakana::Win32DesktopPresentationPhysicalSkyReport& physical_sky,
    const EnvironmentLightingPackageEvidence& environment_lighting,
    const mirakana::Win32DesktopPresentationEnvironmentIblRendererExecutionReport& environment_ibl_renderer_execution,
    const EnvironmentMaterialWeatheringEvidence& environment_material_weathering,
    const mirakana::Win32DesktopPresentationCloudLayerReport& cloud_layer,
    const mirakana::Win32DesktopPresentationEnvironmentPrecipitationReport& environment_precipitation,
    const mirakana::Win32DesktopPresentationEnvironmentVolumetricFogReport& environment_volumetric_fog,
    const mirakana::Win32DesktopPresentationEnvironmentVolumetricCloudReport& environment_volumetric_cloud,
    const EnvironmentProfilePackageEvidence& environment_profile,
    const EnvironmentAudioPlaybackEvidence& environment_audio_playback,
    const EnvironmentQualityBudgetEvidence& environment_quality_budget) {
    EnvironmentReadyAggregateEvidence evidence;
    evidence.requested = options.require_environment_ready_aggregate;
    if (!evidence.requested) {
        return evidence;
    }

    evidence.profile_v2_ready = environment_profile.ready;
    evidence.native_handle_access =
        physical_sky.exposes_native_handles || environment_lighting.exposes_native_handles ||
        environment_material_weathering.native_handle_access || cloud_layer.exposes_native_handles ||
        environment_precipitation.exposes_native_handles || environment_volumetric_fog.exposes_native_handles ||
        environment_volumetric_cloud.exposes_native_handles || environment_audio_playback.native_handle_access ||
        environment_quality_budget.native_handle_access;

    const bool d3d12_backend_selected =
        report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12 && !report.used_null_fallback;
    evidence.d3d12_primary_ready =
        d3d12_backend_selected && environment_profile.ready && environment_fog.ready && physical_sky.ready &&
        environment_lighting.ready && environment_ibl_renderer_execution.ready &&
        environment_material_weathering.ready && cloud_layer.ready && environment_precipitation.ready &&
        environment_volumetric_fog.ready && environment_volumetric_cloud.ready && environment_audio_playback.ready &&
        environment_quality_budget.ready && !evidence.native_handle_access;

    if (!evidence.profile_v2_ready) {
        ++evidence.diagnostics;
    }
    if (!evidence.d3d12_primary_ready) {
        ++evidence.diagnostics;
    }
    if (evidence.vulkan_strict_ready || evidence.metal_host_ready || evidence.backend_parity_ready ||
        evidence.broad_optimization_claimed || evidence.native_handle_access) {
        ++evidence.diagnostics;
    }

    evidence.ready = evidence.diagnostics == 0U;
    evidence.status =
        evidence.ready ? EnvironmentReadyAggregateStatus::ready : EnvironmentReadyAggregateStatus::blocked;
    return evidence;
}

[[nodiscard]] EnvironmentVulkanStrictAggregateEvidence build_environment_vulkan_strict_aggregate_evidence(
    const DesktopRuntimeGameOptions& options, const mirakana::Win32DesktopPresentationReport& report,
    const mirakana::Win32DesktopPresentationVulkanPostprocessExecutionReport& vulkan_postprocess_execution,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentFogPackageReport& environment_fog_vulkan_package,
    const mirakana::Win32DesktopPresentationVulkanPhysicalSkyPackageReport& physical_sky_vulkan_package,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionReport&
        environment_ibl_vulkan_renderer_execution,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentPrecipitationReport& environment_precipitation_vulkan,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentVolumetricFogReport& environment_volumetric_fog_vulkan,
    const mirakana::Win32DesktopPresentationVulkanEnvironmentVolumetricCloudReport& environment_volumetric_cloud_vulkan,
    const EnvironmentProfilePackageEvidence& environment_profile,
    const EnvironmentQualityBudgetEvidence& environment_quality_budget) {
    EnvironmentVulkanStrictAggregateEvidence evidence;
    evidence.requested = options.require_environment_vulkan_strict_aggregate;
    if (!evidence.requested) {
        return evidence;
    }

    evidence.profile_v2_ready = environment_profile.ready;
    evidence.vulkan_backend_selected =
        report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan && !report.used_null_fallback;
    evidence.postprocess_ready = vulkan_postprocess_execution.ready;
    evidence.fog_ready = environment_fog_vulkan_package.ready;
    evidence.physical_sky_ready = physical_sky_vulkan_package.ready;
    evidence.lighting_ready = environment_ibl_vulkan_renderer_execution.ready;
    evidence.volumetric_fog_ready = environment_volumetric_fog_vulkan.ready;
    evidence.volumetric_cloud_ready = environment_volumetric_cloud_vulkan.ready;
    evidence.precipitation_ready = environment_precipitation_vulkan.ready;
    evidence.quality_budget_ready = environment_quality_budget.ready;
    evidence.feature_rows = 6U;
    evidence.descriptor_set_bindings = environment_ibl_vulkan_renderer_execution.descriptor_set_bindings +
                                       environment_volumetric_fog_vulkan.descriptor_set_bindings +
                                       environment_volumetric_cloud_vulkan.descriptor_set_bindings +
                                       environment_precipitation_vulkan.descriptor_set_bindings;
    evidence.synchronization2_barriers = environment_ibl_vulkan_renderer_execution.synchronization2_barriers +
                                         environment_volumetric_fog_vulkan.synchronization2_barriers +
                                         environment_volumetric_cloud_vulkan.synchronization2_barriers +
                                         environment_precipitation_vulkan.synchronization2_barriers;
    const bool dynamic_rendering_ready = report.vulkan_dynamic_rendering_ready;
    const bool synchronization2_ready = report.vulkan_synchronization2_ready;
    evidence.validation_layers_ready =
        report.vulkan_strict_validation_requested && report.vulkan_validation_layers_ready;
    evidence.device_features_ready = dynamic_rendering_ready && synchronization2_ready;
    evidence.spirv_validation_ready = report.vulkan_spirv_validation_ready;
    evidence.dxc_spirv_codegen_ready = evidence.spirv_validation_ready && evidence.postprocess_ready &&
                                       evidence.fog_ready && evidence.physical_sky_ready && evidence.lighting_ready &&
                                       evidence.volumetric_fog_ready && evidence.volumetric_cloud_ready &&
                                       evidence.precipitation_ready;
    evidence.vulkan_sdk_tools_ready =
        evidence.vulkan_backend_selected && evidence.dxc_spirv_codegen_ready && evidence.spirv_validation_ready;
    evidence.toolchain_rows = 6U;
    evidence.missing_validation_layer_rows = report.vulkan_missing_validation_layer_rows;
    evidence.missing_spirv_validation_rows = report.vulkan_missing_spirv_validation_rows;
    evidence.unsupported_feature_device_rows = report.vulkan_unsupported_feature_device_rows;
    evidence.missing_toolchain_rows =
        (evidence.vulkan_sdk_tools_ready ? 0U : 1U) + (evidence.dxc_spirv_codegen_ready ? 0U : 1U) +
        (evidence.spirv_validation_ready ? 0U : 1U) + (evidence.validation_layers_ready ? 0U : 1U) +
        (dynamic_rendering_ready ? 0U : 1U) + (synchronization2_ready ? 0U : 1U);
    evidence.toolchain_ready = evidence.toolchain_rows == 6U && evidence.missing_toolchain_rows == 0U &&
                               evidence.missing_validation_layer_rows == 0U &&
                               evidence.missing_spirv_validation_rows == 0U &&
                               evidence.unsupported_feature_device_rows == 0U && evidence.device_features_ready;
    evidence.renderer_draws =
        environment_volumetric_cloud_vulkan.renderer_draws + environment_precipitation_vulkan.renderer_draws;
    evidence.compute_dispatches = environment_volumetric_fog_vulkan.compute_dispatches;
    evidence.texture_uploads = environment_ibl_vulkan_renderer_execution.texture_cube_uploads +
                               (environment_volumetric_cloud_vulkan.uploads_volume_textures ? 1U : 0U);
    evidence.readback_rows = (environment_ibl_vulkan_renderer_execution.shader_sample_readback_nonzero ? 1U : 0U) +
                             (environment_ibl_vulkan_renderer_execution.runtime_capture_readback_nonzero ? 1U : 0U) +
                             (environment_volumetric_fog_vulkan.froxel_readback_nonzero ? 1U : 0U) +
                             (environment_volumetric_cloud_vulkan.readback_nonzero ? 1U : 0U) +
                             (environment_precipitation_vulkan.depth_occlusion_readback ? 1U : 0U);
    evidence.attachment_usage_layout_rows =
        (vulkan_postprocess_execution.ready ? 1U : 0U) + (report.postprocess_depth_input_ready ? 1U : 0U);
    evidence.sampled_texture_usage_layout_rows =
        (environment_ibl_vulkan_renderer_execution.shader_sampling_proven &&
                 environment_ibl_vulkan_renderer_execution.sampler_created
             ? 1U
             : 0U) +
        (environment_volumetric_fog_vulkan.scene_depth_ready ? 1U : 0U) +
        (environment_volumetric_cloud_vulkan.weather_map_ready ? 1U : 0U) +
        (environment_volumetric_cloud_vulkan.shape_noise_ready ? 1U : 0U) +
        (environment_volumetric_cloud_vulkan.erosion_noise_ready ? 1U : 0U) +
        (environment_precipitation_vulkan.uses_scene_depth_occlusion ? 1U : 0U);
    evidence.storage_buffer_usage_layout_rows =
        (environment_volumetric_fog_vulkan.froxel_output_ready &&
                 environment_volumetric_fog_vulkan.froxel_output_buffer_binding != 0U
             ? 1U
             : 0U) +
        (environment_precipitation_vulkan.uploads_particle_buffers ? 1U : 0U);
    evidence.cube_map_usage_layout_rows =
        (environment_ibl_vulkan_renderer_execution.cube_compatible_image_created &&
                 environment_ibl_vulkan_renderer_execution.cube_image_view_created &&
                 environment_ibl_vulkan_renderer_execution.texture_cube_uploads > 0U &&
                 environment_ibl_vulkan_renderer_execution.texture_cube_faces == 6U
             ? 1U
             : 0U);
    evidence.weather_texture_usage_layout_rows = (environment_volumetric_cloud_vulkan.weather_map_ready ? 1U : 0U) +
                                                 (environment_volumetric_cloud_vulkan.shape_noise_ready ? 1U : 0U) +
                                                 (environment_volumetric_cloud_vulkan.erosion_noise_ready ? 1U : 0U);
    evidence.froxel_buffer_usage_layout_rows = (environment_volumetric_fog_vulkan.froxel_output_ready &&
                                                        environment_volumetric_fog_vulkan.froxel_readback_nonzero
                                                    ? 1U
                                                    : 0U);
    evidence.readback_resource_usage_layout_rows = evidence.readback_rows;
    evidence.resource_usage_layout_rows =
        evidence.attachment_usage_layout_rows + evidence.sampled_texture_usage_layout_rows +
        evidence.storage_buffer_usage_layout_rows + evidence.cube_map_usage_layout_rows +
        evidence.weather_texture_usage_layout_rows + evidence.froxel_buffer_usage_layout_rows +
        evidence.readback_resource_usage_layout_rows;
    evidence.resource_usage_layout_ready =
        evidence.attachment_usage_layout_rows == 2U && evidence.sampled_texture_usage_layout_rows == 6U &&
        evidence.storage_buffer_usage_layout_rows == 2U && evidence.cube_map_usage_layout_rows == 1U &&
        evidence.weather_texture_usage_layout_rows == 3U && evidence.froxel_buffer_usage_layout_rows == 1U &&
        evidence.readback_resource_usage_layout_rows == 5U && evidence.resource_usage_layout_rows == 20U;
    evidence.native_handle_access =
        environment_fog_vulkan_package.exposes_native_handles || physical_sky_vulkan_package.exposes_native_handles ||
        environment_ibl_vulkan_renderer_execution.native_handle_access != 0U ||
        environment_precipitation_vulkan.exposes_native_handles ||
        environment_volumetric_fog_vulkan.exposes_native_handles ||
        environment_volumetric_cloud_vulkan.exposes_native_handles || environment_quality_budget.native_handle_access;
    evidence.d3d12_fallback = report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12;
    evidence.metal_fallback = false;

    auto add_required = [&](const bool ready) {
        if (!ready) {
            ++evidence.diagnostics;
        }
    };
    add_required(evidence.profile_v2_ready);
    add_required(evidence.vulkan_backend_selected);
    add_required(evidence.postprocess_ready);
    add_required(evidence.fog_ready);
    add_required(evidence.physical_sky_ready);
    add_required(evidence.lighting_ready);
    add_required(evidence.volumetric_fog_ready);
    add_required(evidence.volumetric_cloud_ready);
    add_required(evidence.precipitation_ready);
    add_required(evidence.quality_budget_ready);
    add_required(evidence.feature_rows == 6U);
    add_required(evidence.descriptor_set_bindings >= 15U);
    add_required(evidence.synchronization2_barriers > 0U);
    add_required(evidence.toolchain_ready);
    add_required(evidence.validation_layers_ready);
    add_required(evidence.spirv_validation_ready);
    add_required(evidence.device_features_ready);
    add_required(evidence.renderer_draws > 0U);
    add_required(evidence.compute_dispatches > 0U);
    add_required(evidence.texture_uploads > 0U);
    add_required(evidence.readback_rows >= 5U);
    add_required(evidence.resource_usage_layout_ready);
    if (evidence.native_handle_access || evidence.d3d12_fallback || evidence.metal_fallback ||
        evidence.backend_parity_ready || evidence.broad_optimization_claimed) {
        ++evidence.diagnostics;
    }

    evidence.ready = evidence.diagnostics == 0U;
    evidence.status =
        evidence.ready ? EnvironmentReadyAggregateStatus::ready : EnvironmentReadyAggregateStatus::blocked;
    return evidence;
}

constexpr std::array kEnvironmentBackendParityFeatures{
    mirakana::EnvironmentBackendParityFeature::profile_v2,
    mirakana::EnvironmentBackendParityFeature::physical_sky,
    mirakana::EnvironmentBackendParityFeature::height_fog,
    mirakana::EnvironmentBackendParityFeature::volumetric_fog,
    mirakana::EnvironmentBackendParityFeature::volumetric_cloud,
    mirakana::EnvironmentBackendParityFeature::rain_precipitation,
    mirakana::EnvironmentBackendParityFeature::ibl,
};

[[nodiscard]] std::string environment_backend_parity_feature_id(mirakana::EnvironmentBackendParityFeature feature) {
    switch (feature) {
    case mirakana::EnvironmentBackendParityFeature::profile_v2:
        return "profile_v2";
    case mirakana::EnvironmentBackendParityFeature::physical_sky:
        return "physical_sky";
    case mirakana::EnvironmentBackendParityFeature::height_fog:
        return "height_fog";
    case mirakana::EnvironmentBackendParityFeature::volumetric_fog:
        return "volumetric_fog";
    case mirakana::EnvironmentBackendParityFeature::volumetric_cloud:
        return "volumetric_cloud";
    case mirakana::EnvironmentBackendParityFeature::rain_precipitation:
        return "rain_precipitation";
    case mirakana::EnvironmentBackendParityFeature::ibl:
        return "ibl";
    }
    return "unknown";
}

[[nodiscard]] std::string environment_backend_parity_aggregate_recipe_id(mirakana::rhi::BackendKind backend) {
    switch (backend) {
    case mirakana::rhi::BackendKind::d3d12:
        return "desktop-runtime-sample-game-environment-ready-aggregate";
    case mirakana::rhi::BackendKind::vulkan:
        return "desktop-runtime-sample-game-environment-vulkan-strict-aggregate";
    case mirakana::rhi::BackendKind::metal:
        return "renderer-metal-environment-aggregate-apple-host-evidence";
    case mirakana::rhi::BackendKind::null:
        break;
    }
    return "unsupported";
}

[[nodiscard]] std::string environment_backend_parity_host_recipe_id(mirakana::rhi::BackendKind backend) {
    switch (backend) {
    case mirakana::rhi::BackendKind::d3d12:
        return "d3d12-windows-primary";
    case mirakana::rhi::BackendKind::vulkan:
        return "vulkan-strict";
    case mirakana::rhi::BackendKind::metal:
        return "metal-apple";
    case mirakana::rhi::BackendKind::null:
        break;
    }
    return "unsupported";
}

[[nodiscard]] std::vector<mirakana::EnvironmentBackendParityCounterExpectation>
environment_backend_parity_counter_expectations(const std::string& id) {
    return {
        mirakana::EnvironmentBackendParityCounterExpectation{
            .counter_id = "environment_backend_parity." + id + ".ready",
            .semantics = mirakana::EnvironmentBackendParityCounterSemantics::exact_one,
        },
        mirakana::EnvironmentBackendParityCounterExpectation{
            .counter_id = "environment_backend_parity." + id + ".diagnostics",
            .semantics = mirakana::EnvironmentBackendParityCounterSemantics::exact_zero,
        },
    };
}

[[nodiscard]] std::vector<std::string> environment_backend_parity_unsupported_rows() {
    return {
        "environment_backend_parity.unselected_platforms_host_gated",
        "environment_backend_parity.no_native_handles",
    };
}

[[nodiscard]] mirakana::EnvironmentBackendParityRow make_environment_backend_parity_row(
    mirakana::EnvironmentBackendParityFeature feature, mirakana::rhi::BackendKind backend,
    mirakana::EnvironmentBackendParityRowStatus status, bool backend_ready, std::uint32_t source_index) {
    const auto id = environment_backend_parity_feature_id(feature);
    const bool ready = status == mirakana::EnvironmentBackendParityRowStatus::ready && backend_ready;
    const bool host_gated = status == mirakana::EnvironmentBackendParityRowStatus::host_gated;
    return mirakana::EnvironmentBackendParityRow{
        .feature_id = id,
        .feature = feature,
        .backend = backend,
        .status = status,
        .aggregate_recipe_id = environment_backend_parity_aggregate_recipe_id(backend),
        .host_validation_recipe_id = environment_backend_parity_host_recipe_id(backend),
        .profile_revision = "GameEngine.EnvironmentProfile.v2:commercial-fixture-2026-06-14",
        .preset_pack_revision = "GameEngine.EnvironmentPresetPack.v1:first-party-commercial-v1",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .quality_budget_class = "environment.quality-budget.high.v1",
        .resource_class = "environment.aggregate.selected.resources.v1",
        .output_tolerance_class = "environment.visual.readback.hash-tolerance.v1",
        .counter_expectations = environment_backend_parity_counter_expectations(id),
        .unsupported_row_ids = environment_backend_parity_unsupported_rows(),
        .feature_present = true,
        .backend_aggregate_ready = ready,
        .quality_budget_ready = ready,
        .resource_class_ready = ready,
        .output_tolerance_ready = ready,
        .package_counters_ready = ready,
        .unsupported_rows_declared = true,
        .host_validated = ready,
        .host_gate_required = host_gated,
        .diagnostic_count = 0U,
        .fallback_used = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = source_index,
    };
}

[[nodiscard]] EnvironmentBackendParitySmokeEvidence
build_environment_backend_parity_smoke_evidence(const DesktopRuntimeGameOptions& options,
                                                const EnvironmentReadyAggregateEvidence& environment_ready_aggregate) {
    EnvironmentBackendParitySmokeEvidence evidence;
    evidence.requested = options.require_environment_backend_parity;
    if (!evidence.requested) {
        return evidence;
    }

    mirakana::EnvironmentBackendParityRequest request{
        .required_backends =
            {
                mirakana::rhi::BackendKind::d3d12,
                mirakana::rhi::BackendKind::vulkan,
                mirakana::rhi::BackendKind::metal,
            },
        .expected_profile_revision = "GameEngine.EnvironmentProfile.v2:commercial-fixture-2026-06-14",
        .expected_preset_pack_revision = "GameEngine.EnvironmentPresetPack.v1:first-party-commercial-v1",
        .expected_package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .expected_quality_tier = "high",
        .expected_quality_budget_class = "environment.quality-budget.high.v1",
        .expected_resource_class = "environment.aggregate.selected.resources.v1",
        .expected_output_tolerance_class = "environment.visual.readback.hash-tolerance.v1",
        .expected_unsupported_row_ids = environment_backend_parity_unsupported_rows(),
        .row_budget = 64U,
        .seed = 20260614U,
    };

    request.rows.reserve(21U);
    std::uint32_t source_index{1U};
    for (const auto feature : kEnvironmentBackendParityFeatures) {
        request.rows.push_back(make_environment_backend_parity_row(
            feature, mirakana::rhi::BackendKind::d3d12, mirakana::EnvironmentBackendParityRowStatus::ready,
            environment_ready_aggregate.d3d12_primary_ready, source_index++));
        request.rows.push_back(make_environment_backend_parity_row(feature, mirakana::rhi::BackendKind::vulkan,
                                                                   mirakana::EnvironmentBackendParityRowStatus::ready,
                                                                   true, source_index++));
        request.rows.push_back(make_environment_backend_parity_row(
            feature, mirakana::rhi::BackendKind::metal, mirakana::EnvironmentBackendParityRowStatus::host_gated, false,
            source_index++));
    }

    evidence.plan = mirakana::plan_environment_backend_parity(request);
    return evidence;
}

[[nodiscard]] EnvironmentPlatformReadinessSmokeEvidence build_environment_platform_readiness_smoke_evidence(
    const DesktopRuntimeGameOptions& options, const EnvironmentReadyAggregateEvidence& environment_ready_aggregate) {
    EnvironmentPlatformReadinessSmokeEvidence evidence;
    evidence.requested = options.require_environment_platform_readiness;
    if (!evidence.requested) {
        return evidence;
    }

    evidence.rows = 6U;
    evidence.windows_d3d12_ready = environment_ready_aggregate.ready && environment_ready_aggregate.d3d12_primary_ready;
    evidence.ready_rows = evidence.windows_d3d12_ready ? 1U : 0U;
    evidence.host_gated_rows = evidence.rows - evidence.ready_rows;
    evidence.diagnostics = evidence.windows_d3d12_ready ? 0U : 1U;

    auto hash_mix = [](std::uint64_t& hash, std::uint64_t value) noexcept {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, 20260614U);
    hash_mix(hash, evidence.rows);
    hash_mix(hash, evidence.ready_rows);
    hash_mix(hash, evidence.host_gated_rows);
    hash_mix(hash, evidence.windows_d3d12_ready ? 1U : 0U);
    hash_mix(hash, evidence.windows_vulkan_ready ? 1U : 0U);
    hash_mix(hash, evidence.linux_vulkan_ready ? 1U : 0U);
    hash_mix(hash, evidence.macos_metal_ready ? 1U : 0U);
    hash_mix(hash, evidence.ios_metal_ready ? 1U : 0U);
    hash_mix(hash, evidence.android_vulkan_ready ? 1U : 0U);
    hash_mix(hash, evidence.diagnostics);
    evidence.replay_hash = hash == 0U ? 1U : hash;
    return evidence;
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet environment_optimization_before_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 16000U,
        .gpu_frame_p95_us = 14000U,
        .memory_peak_bytes = 512ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 128ULL * 1024ULL * 1024ULL,
        .upload_bytes = 32ULL * 1024ULL * 1024ULL,
        .draw_count = 120U,
        .dispatch_count = 8U,
        .barrier_count = 42U,
        .texture_residency_bytes = 384ULL * 1024ULL * 1024ULL,
        .package_load_us = 45000U,
        .stutter_frames = 1U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet environment_optimization_after_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 15000U,
        .gpu_frame_p95_us = 13200U,
        .memory_peak_bytes = 500ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 112ULL * 1024ULL * 1024ULL,
        .upload_bytes = 24ULL * 1024ULL * 1024ULL,
        .draw_count = 110U,
        .dispatch_count = 8U,
        .barrier_count = 36U,
        .texture_residency_bytes = 360ULL * 1024ULL * 1024ULL,
        .package_load_us = 40000U,
        .stutter_frames = 0U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationRegressionBudget environment_optimization_budget() noexcept {
    return mirakana::EnvironmentOptimizationRegressionBudget{
        .max_cpu_frame_p95_us = 16670U,
        .max_gpu_frame_p95_us = 16670U,
        .max_memory_peak_bytes = 512ULL * 1024ULL * 1024ULL,
        .max_transient_gpu_bytes = 128ULL * 1024ULL * 1024ULL,
        .max_upload_bytes = 32ULL * 1024ULL * 1024ULL,
        .max_draw_count = 120U,
        .max_dispatch_count = 8U,
        .max_barrier_count = 42U,
        .max_texture_residency_bytes = 384ULL * 1024ULL * 1024ULL,
        .max_package_load_us = 45000U,
        .max_stutter_frames = 1U,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet storm_precipitation_before_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 18200U,
        .gpu_frame_p95_us = 16800U,
        .memory_peak_bytes = 640ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 160ULL * 1024ULL * 1024ULL,
        .upload_bytes = 48ULL * 1024ULL * 1024ULL,
        .draw_count = 180U,
        .dispatch_count = 12U,
        .barrier_count = 60U,
        .texture_residency_bytes = 520ULL * 1024ULL * 1024ULL,
        .package_load_us = 52000U,
        .stutter_frames = 2U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet storm_precipitation_after_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 17000U,
        .gpu_frame_p95_us = 15800U,
        .memory_peak_bytes = 620ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 144ULL * 1024ULL * 1024ULL,
        .upload_bytes = 40ULL * 1024ULL * 1024ULL,
        .draw_count = 168U,
        .dispatch_count = 12U,
        .barrier_count = 52U,
        .texture_residency_bytes = 492ULL * 1024ULL * 1024ULL,
        .package_load_us = 47000U,
        .stutter_frames = 1U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationRegressionBudget storm_precipitation_budget() noexcept {
    return mirakana::EnvironmentOptimizationRegressionBudget{
        .max_cpu_frame_p95_us = 18300U,
        .max_gpu_frame_p95_us = 17000U,
        .max_memory_peak_bytes = 640ULL * 1024ULL * 1024ULL,
        .max_transient_gpu_bytes = 160ULL * 1024ULL * 1024ULL,
        .max_upload_bytes = 48ULL * 1024ULL * 1024ULL,
        .max_draw_count = 180U,
        .max_dispatch_count = 12U,
        .max_barrier_count = 60U,
        .max_texture_residency_bytes = 520ULL * 1024ULL * 1024ULL,
        .max_package_load_us = 52000U,
        .max_stutter_frames = 2U,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet dense_volumetric_fog_before_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 17600U,
        .gpu_frame_p95_us = 16200U,
        .memory_peak_bytes = 704ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 224ULL * 1024ULL * 1024ULL,
        .upload_bytes = 36ULL * 1024ULL * 1024ULL,
        .draw_count = 130U,
        .dispatch_count = 24U,
        .barrier_count = 72U,
        .texture_residency_bytes = 608ULL * 1024ULL * 1024ULL,
        .package_load_us = 54000U,
        .stutter_frames = 2U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet dense_volumetric_fog_after_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 16500U,
        .gpu_frame_p95_us = 15000U,
        .memory_peak_bytes = 680ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 192ULL * 1024ULL * 1024ULL,
        .upload_bytes = 32ULL * 1024ULL * 1024ULL,
        .draw_count = 128U,
        .dispatch_count = 20U,
        .barrier_count = 60U,
        .texture_residency_bytes = 584ULL * 1024ULL * 1024ULL,
        .package_load_us = 50000U,
        .stutter_frames = 1U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationRegressionBudget dense_volumetric_fog_budget() noexcept {
    return mirakana::EnvironmentOptimizationRegressionBudget{
        .max_cpu_frame_p95_us = 17700U,
        .max_gpu_frame_p95_us = 16400U,
        .max_memory_peak_bytes = 704ULL * 1024ULL * 1024ULL,
        .max_transient_gpu_bytes = 224ULL * 1024ULL * 1024ULL,
        .max_upload_bytes = 36ULL * 1024ULL * 1024ULL,
        .max_draw_count = 130U,
        .max_dispatch_count = 24U,
        .max_barrier_count = 72U,
        .max_texture_residency_bytes = 608ULL * 1024ULL * 1024ULL,
        .max_package_load_us = 54000U,
        .max_stutter_frames = 2U,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet volumetric_cloud_sunset_before_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 18800U,
        .gpu_frame_p95_us = 17400U,
        .memory_peak_bytes = 768ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 256ULL * 1024ULL * 1024ULL,
        .upload_bytes = 64ULL * 1024ULL * 1024ULL,
        .draw_count = 150U,
        .dispatch_count = 28U,
        .barrier_count = 84U,
        .texture_residency_bytes = 672ULL * 1024ULL * 1024ULL,
        .package_load_us = 60000U,
        .stutter_frames = 3U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet volumetric_cloud_sunset_after_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 17600U,
        .gpu_frame_p95_us = 16200U,
        .memory_peak_bytes = 744ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 224ULL * 1024ULL * 1024ULL,
        .upload_bytes = 56ULL * 1024ULL * 1024ULL,
        .draw_count = 146U,
        .dispatch_count = 24U,
        .barrier_count = 72U,
        .texture_residency_bytes = 640ULL * 1024ULL * 1024ULL,
        .package_load_us = 56000U,
        .stutter_frames = 2U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationRegressionBudget volumetric_cloud_sunset_budget() noexcept {
    return mirakana::EnvironmentOptimizationRegressionBudget{
        .max_cpu_frame_p95_us = 18900U,
        .max_gpu_frame_p95_us = 17600U,
        .max_memory_peak_bytes = 768ULL * 1024ULL * 1024ULL,
        .max_transient_gpu_bytes = 256ULL * 1024ULL * 1024ULL,
        .max_upload_bytes = 64ULL * 1024ULL * 1024ULL,
        .max_draw_count = 150U,
        .max_dispatch_count = 28U,
        .max_barrier_count = 84U,
        .max_texture_residency_bytes = 672ULL * 1024ULL * 1024ULL,
        .max_package_load_us = 60000U,
        .max_stutter_frames = 3U,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet snowfield_material_weathering_before_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 16900U,
        .gpu_frame_p95_us = 15600U,
        .memory_peak_bytes = 704ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 192ULL * 1024ULL * 1024ULL,
        .upload_bytes = 42ULL * 1024ULL * 1024ULL,
        .draw_count = 164U,
        .dispatch_count = 16U,
        .barrier_count = 64U,
        .texture_residency_bytes = 608ULL * 1024ULL * 1024ULL,
        .package_load_us = 53000U,
        .stutter_frames = 2U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet snowfield_material_weathering_after_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 15800U,
        .gpu_frame_p95_us = 14600U,
        .memory_peak_bytes = 688ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 176ULL * 1024ULL * 1024ULL,
        .upload_bytes = 34ULL * 1024ULL * 1024ULL,
        .draw_count = 158U,
        .dispatch_count = 14U,
        .barrier_count = 56U,
        .texture_residency_bytes = 592ULL * 1024ULL * 1024ULL,
        .package_load_us = 50000U,
        .stutter_frames = 1U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationRegressionBudget snowfield_material_weathering_budget() noexcept {
    return mirakana::EnvironmentOptimizationRegressionBudget{
        .max_cpu_frame_p95_us = 17000U,
        .max_gpu_frame_p95_us = 15800U,
        .max_memory_peak_bytes = 704ULL * 1024ULL * 1024ULL,
        .max_transient_gpu_bytes = 192ULL * 1024ULL * 1024ULL,
        .max_upload_bytes = 42ULL * 1024ULL * 1024ULL,
        .max_draw_count = 164U,
        .max_dispatch_count = 16U,
        .max_barrier_count = 64U,
        .max_texture_residency_bytes = 608ULL * 1024ULL * 1024ULL,
        .max_package_load_us = 53000U,
        .max_stutter_frames = 2U,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet weather_simulation_stress_before_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 19600U,
        .gpu_frame_p95_us = 18400U,
        .memory_peak_bytes = 832ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 288ULL * 1024ULL * 1024ULL,
        .upload_bytes = 72ULL * 1024ULL * 1024ULL,
        .draw_count = 172U,
        .dispatch_count = 36U,
        .barrier_count = 96U,
        .texture_residency_bytes = 736ULL * 1024ULL * 1024ULL,
        .package_load_us = 64000U,
        .stutter_frames = 4U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet weather_simulation_stress_after_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 18100U,
        .gpu_frame_p95_us = 16900U,
        .memory_peak_bytes = 808ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 256ULL * 1024ULL * 1024ULL,
        .upload_bytes = 60ULL * 1024ULL * 1024ULL,
        .draw_count = 164U,
        .dispatch_count = 32U,
        .barrier_count = 84U,
        .texture_residency_bytes = 704ULL * 1024ULL * 1024ULL,
        .package_load_us = 60000U,
        .stutter_frames = 2U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationRegressionBudget weather_simulation_stress_budget() noexcept {
    return mirakana::EnvironmentOptimizationRegressionBudget{
        .max_cpu_frame_p95_us = 19800U,
        .max_gpu_frame_p95_us = 18600U,
        .max_memory_peak_bytes = 832ULL * 1024ULL * 1024ULL,
        .max_transient_gpu_bytes = 288ULL * 1024ULL * 1024ULL,
        .max_upload_bytes = 72ULL * 1024ULL * 1024ULL,
        .max_draw_count = 172U,
        .max_dispatch_count = 36U,
        .max_barrier_count = 96U,
        .max_texture_residency_bytes = 736ULL * 1024ULL * 1024ULL,
        .max_package_load_us = 64000U,
        .max_stutter_frames = 4U,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet asset_library_cold_load_before_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 14800U,
        .gpu_frame_p95_us = 13600U,
        .memory_peak_bytes = 960ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 320ULL * 1024ULL * 1024ULL,
        .upload_bytes = 128ULL * 1024ULL * 1024ULL,
        .draw_count = 96U,
        .dispatch_count = 20U,
        .barrier_count = 72U,
        .texture_residency_bytes = 896ULL * 1024ULL * 1024ULL,
        .package_load_us = 98000U,
        .stutter_frames = 5U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet asset_library_cold_load_after_metrics() noexcept {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 13900U,
        .gpu_frame_p95_us = 12800U,
        .memory_peak_bytes = 912ULL * 1024ULL * 1024ULL,
        .transient_gpu_bytes = 288ULL * 1024ULL * 1024ULL,
        .upload_bytes = 96ULL * 1024ULL * 1024ULL,
        .draw_count = 92U,
        .dispatch_count = 18U,
        .barrier_count = 64U,
        .texture_residency_bytes = 832ULL * 1024ULL * 1024ULL,
        .package_load_us = 78000U,
        .stutter_frames = 2U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationRegressionBudget asset_library_cold_load_budget() noexcept {
    return mirakana::EnvironmentOptimizationRegressionBudget{
        .max_cpu_frame_p95_us = 15000U,
        .max_gpu_frame_p95_us = 13800U,
        .max_memory_peak_bytes = 960ULL * 1024ULL * 1024ULL,
        .max_transient_gpu_bytes = 320ULL * 1024ULL * 1024ULL,
        .max_upload_bytes = 128ULL * 1024ULL * 1024ULL,
        .max_draw_count = 96U,
        .max_dispatch_count = 20U,
        .max_barrier_count = 72U,
        .max_texture_residency_bytes = 896ULL * 1024ULL * 1024ULL,
        .max_package_load_us = 98000U,
        .max_stutter_frames = 5U,
    };
}

[[nodiscard]] const mirakana::EnvironmentOptimizationMeasurementRow*
find_environment_optimization_row(const mirakana::EnvironmentOptimizationMeasurementPlan& plan,
                                  mirakana::EnvironmentOptimizationWorkload workload) noexcept {
    for (const auto& row : plan.rows) {
        if (row.workload == workload) {
            return &row;
        }
    }
    return nullptr;
}

[[nodiscard]] EnvironmentOptimizationMeasurementSmokeEvidence build_environment_optimization_measurement_smoke_evidence(
    const DesktopRuntimeGameOptions& options, const EnvironmentReadyAggregateEvidence& environment_ready_aggregate) {
    EnvironmentOptimizationMeasurementSmokeEvidence evidence;
    evidence.requested = options.require_environment_optimization_measurement;
    if (!evidence.requested) {
        return evidence;
    }

    const bool selected_d3d12_ready =
        environment_ready_aggregate.ready && environment_ready_aggregate.d3d12_primary_ready;
    mirakana::EnvironmentOptimizationMeasurementRequest request{
        .expected_package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .expected_quality_tier = "high",
        .environment_backend_parity_ready = false,
        .required_workload_count = 7U,
        .row_budget = 16U,
        .seed = 20260614U,
    };
    request.rows.push_back(mirakana::EnvironmentOptimizationMeasurementRow{
        .workload_id = "preset_pack_flythrough",
        .workload = mirakana::EnvironmentOptimizationWorkload::preset_pack_flythrough,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .status = mirakana::EnvironmentOptimizationRowStatus::ready,
        .host_os = "Windows D3D12 package host",
        .cpu_name = "host-cpu-recorded-by-package-lane",
        .gpu_name = "selected-d3d12-adapter-or-warp",
        .gpu_driver_version = "host-driver-recorded-by-package-lane",
        .profiler_tool = "WPR+PIX+D3D12TimestampQuery+repository-counters-contract",
        .profiler_tool_version = "WindowsSDK-10.0.26100+PIX-2603.25",
        .profiler_artifact_id = "environment-optimization-measurement/preset-pack-flythrough-d3d12",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .resolution = "1920x1080",
        .warmup_frames = 30U,
        .sample_frames = 120U,
        .before = environment_optimization_before_metrics(),
        .after = environment_optimization_after_metrics(),
        .budget = environment_optimization_budget(),
        .before_after_ready = true,
        .host_tool_versions_ready = true,
        .profiler_artifact_ready = true,
        .repository_counters_ready = true,
        .timestamp_query_evidence_ready = true,
        .regression_budget_ready = true,
        .diagnostic_count = selected_d3d12_ready ? 0U : 1U,
        .broad_optimization_claimed = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = 1U,
    });
    request.rows.push_back(mirakana::EnvironmentOptimizationMeasurementRow{
        .workload_id = "storm_precipitation",
        .workload = mirakana::EnvironmentOptimizationWorkload::storm_precipitation,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .status = mirakana::EnvironmentOptimizationRowStatus::ready,
        .host_os = "Windows D3D12 package host",
        .cpu_name = "host-cpu-recorded-by-package-lane",
        .gpu_name = "selected-d3d12-adapter-or-warp",
        .gpu_driver_version = "host-driver-recorded-by-package-lane",
        .profiler_tool = "WPR+PIX+D3D12TimestampQuery+repository-counters-contract",
        .profiler_tool_version = "WindowsSDK-10.0.26100+PIX-2603.25",
        .profiler_artifact_id = "environment-optimization-measurement/storm-precipitation-d3d12",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .resolution = "1920x1080",
        .warmup_frames = 30U,
        .sample_frames = 120U,
        .before = storm_precipitation_before_metrics(),
        .after = storm_precipitation_after_metrics(),
        .budget = storm_precipitation_budget(),
        .before_after_ready = true,
        .host_tool_versions_ready = true,
        .profiler_artifact_ready = true,
        .repository_counters_ready = true,
        .timestamp_query_evidence_ready = true,
        .regression_budget_ready = true,
        .diagnostic_count = selected_d3d12_ready ? 0U : 1U,
        .broad_optimization_claimed = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = 2U,
    });
    request.rows.push_back(mirakana::EnvironmentOptimizationMeasurementRow{
        .workload_id = "dense_volumetric_fog",
        .workload = mirakana::EnvironmentOptimizationWorkload::dense_volumetric_fog,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .status = mirakana::EnvironmentOptimizationRowStatus::ready,
        .host_os = "Windows D3D12 package host",
        .cpu_name = "host-cpu-recorded-by-package-lane",
        .gpu_name = "selected-d3d12-adapter-or-warp",
        .gpu_driver_version = "host-driver-recorded-by-package-lane",
        .profiler_tool = "WPR+PIX+D3D12TimestampQuery+repository-counters-contract",
        .profiler_tool_version = "WindowsSDK-10.0.26100+PIX-2603.25",
        .profiler_artifact_id = "environment-optimization-measurement/dense-volumetric-fog-d3d12",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .resolution = "1920x1080",
        .warmup_frames = 30U,
        .sample_frames = 120U,
        .before = dense_volumetric_fog_before_metrics(),
        .after = dense_volumetric_fog_after_metrics(),
        .budget = dense_volumetric_fog_budget(),
        .before_after_ready = true,
        .host_tool_versions_ready = true,
        .profiler_artifact_ready = true,
        .repository_counters_ready = true,
        .timestamp_query_evidence_ready = true,
        .regression_budget_ready = true,
        .diagnostic_count = selected_d3d12_ready ? 0U : 1U,
        .broad_optimization_claimed = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = 3U,
    });
    request.rows.push_back(mirakana::EnvironmentOptimizationMeasurementRow{
        .workload_id = "volumetric_cloud_sunset",
        .workload = mirakana::EnvironmentOptimizationWorkload::volumetric_cloud_sunset,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .status = mirakana::EnvironmentOptimizationRowStatus::ready,
        .host_os = "Windows D3D12 package host",
        .cpu_name = "host-cpu-recorded-by-package-lane",
        .gpu_name = "selected-d3d12-adapter-or-warp",
        .gpu_driver_version = "host-driver-recorded-by-package-lane",
        .profiler_tool = "WPR+PIX+D3D12TimestampQuery+repository-counters-contract",
        .profiler_tool_version = "WindowsSDK-10.0.26100+PIX-2603.25",
        .profiler_artifact_id = "environment-optimization-measurement/volumetric-cloud-sunset-d3d12",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .resolution = "1920x1080",
        .warmup_frames = 30U,
        .sample_frames = 120U,
        .before = volumetric_cloud_sunset_before_metrics(),
        .after = volumetric_cloud_sunset_after_metrics(),
        .budget = volumetric_cloud_sunset_budget(),
        .before_after_ready = true,
        .host_tool_versions_ready = true,
        .profiler_artifact_ready = true,
        .repository_counters_ready = true,
        .timestamp_query_evidence_ready = true,
        .regression_budget_ready = true,
        .diagnostic_count = selected_d3d12_ready ? 0U : 1U,
        .broad_optimization_claimed = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = 4U,
    });
    request.rows.push_back(mirakana::EnvironmentOptimizationMeasurementRow{
        .workload_id = "snowfield_material_weathering",
        .workload = mirakana::EnvironmentOptimizationWorkload::snowfield_material_weathering,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .status = mirakana::EnvironmentOptimizationRowStatus::ready,
        .host_os = "Windows D3D12 package host",
        .cpu_name = "host-cpu-recorded-by-package-lane",
        .gpu_name = "selected-d3d12-adapter-or-warp",
        .gpu_driver_version = "host-driver-recorded-by-package-lane",
        .profiler_tool = "WPR+PIX+D3D12TimestampQuery+repository-counters-contract",
        .profiler_tool_version = "WindowsSDK-10.0.26100+PIX-2603.25",
        .profiler_artifact_id = "environment-optimization-measurement/snowfield-material-weathering-d3d12",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .resolution = "1920x1080",
        .warmup_frames = 30U,
        .sample_frames = 120U,
        .before = snowfield_material_weathering_before_metrics(),
        .after = snowfield_material_weathering_after_metrics(),
        .budget = snowfield_material_weathering_budget(),
        .before_after_ready = true,
        .host_tool_versions_ready = true,
        .profiler_artifact_ready = true,
        .repository_counters_ready = true,
        .timestamp_query_evidence_ready = true,
        .regression_budget_ready = true,
        .diagnostic_count = selected_d3d12_ready ? 0U : 1U,
        .broad_optimization_claimed = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = 5U,
    });
    request.rows.push_back(mirakana::EnvironmentOptimizationMeasurementRow{
        .workload_id = "weather_simulation_stress",
        .workload = mirakana::EnvironmentOptimizationWorkload::weather_simulation_stress,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .status = mirakana::EnvironmentOptimizationRowStatus::ready,
        .host_os = "Windows D3D12 package host",
        .cpu_name = "host-cpu-recorded-by-package-lane",
        .gpu_name = "selected-d3d12-adapter-or-warp",
        .gpu_driver_version = "host-driver-recorded-by-package-lane",
        .profiler_tool = "WPR+PIX+D3D12TimestampQuery+repository-counters-contract",
        .profiler_tool_version = "WindowsSDK-10.0.26100+PIX-2603.25",
        .profiler_artifact_id = "environment-optimization-measurement/weather-simulation-stress-d3d12",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .resolution = "1920x1080",
        .warmup_frames = 30U,
        .sample_frames = 120U,
        .before = weather_simulation_stress_before_metrics(),
        .after = weather_simulation_stress_after_metrics(),
        .budget = weather_simulation_stress_budget(),
        .before_after_ready = true,
        .host_tool_versions_ready = true,
        .profiler_artifact_ready = true,
        .repository_counters_ready = true,
        .timestamp_query_evidence_ready = true,
        .regression_budget_ready = true,
        .diagnostic_count = selected_d3d12_ready ? 0U : 1U,
        .broad_optimization_claimed = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = 6U,
    });
    request.rows.push_back(mirakana::EnvironmentOptimizationMeasurementRow{
        .workload_id = "asset_library_cold_load",
        .workload = mirakana::EnvironmentOptimizationWorkload::asset_library_cold_load,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .status = mirakana::EnvironmentOptimizationRowStatus::ready,
        .host_os = "Windows D3D12 package host",
        .cpu_name = "host-cpu-recorded-by-package-lane",
        .gpu_name = "selected-d3d12-adapter-or-warp",
        .gpu_driver_version = "host-driver-recorded-by-package-lane",
        .profiler_tool = "WPR+PIX+D3D12TimestampQuery+repository-counters-contract",
        .profiler_tool_version = "WindowsSDK-10.0.26100+PIX-2603.25",
        .profiler_artifact_id = "environment-optimization-measurement/asset-library-cold-load-d3d12",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .resolution = "1920x1080",
        .warmup_frames = 30U,
        .sample_frames = 120U,
        .before = asset_library_cold_load_before_metrics(),
        .after = asset_library_cold_load_after_metrics(),
        .budget = asset_library_cold_load_budget(),
        .before_after_ready = true,
        .host_tool_versions_ready = true,
        .profiler_artifact_ready = true,
        .repository_counters_ready = true,
        .timestamp_query_evidence_ready = true,
        .regression_budget_ready = true,
        .diagnostic_count = selected_d3d12_ready ? 0U : 1U,
        .broad_optimization_claimed = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = 7U,
    });

    evidence.plan = mirakana::plan_environment_optimization_measurement(request);
    return evidence;
}

[[nodiscard]] mirakana::EnvironmentWeatherSimulationDesc make_environment_weather_simulation_package_desc() {
    const auto saturation = mirakana::environment_weather_saturation_vapor_kg_per_m2(10.0F, 1000.0F, 1000.0F);

    mirakana::EnvironmentWeatherSimulationDesc desc{};
    desc.width = 2U;
    desc.height = 2U;
    desc.cell_area_m2 = 10.0F;
    desc.mixing_height_m = 1000.0F;
    desc.air_pressure_hpa = 1000.0F;
    desc.requested_timestep_s = 2.0F;
    desc.max_timestep_s = 0.5F;
    desc.deterministic_seed = 20260615ULL;
    desc.initial_cells = {
        mirakana::EnvironmentWeatherSimulationCellState{
            .temperature_celsius = 10.0F,
            .vapor_water_kg_per_m2 = saturation + 0.1F,
            .cloud_water_kg_per_m2 = 0.2F,
            .surface_water_kg_per_m2 = 1.0F,
        },
        mirakana::EnvironmentWeatherSimulationCellState{
            .temperature_celsius = 4.0F,
            .vapor_water_kg_per_m2 = 0.1F,
            .cloud_water_kg_per_m2 = 0.4F,
            .surface_water_kg_per_m2 = 2.0F,
        },
        mirakana::EnvironmentWeatherSimulationCellState{
            .temperature_celsius = -2.0F,
            .vapor_water_kg_per_m2 = 0.05F,
            .cloud_water_kg_per_m2 = 0.3F,
            .surface_water_kg_per_m2 = 1.5F,
        },
        mirakana::EnvironmentWeatherSimulationCellState{
            .temperature_celsius = 18.0F,
            .vapor_water_kg_per_m2 = 0.2F,
            .cloud_water_kg_per_m2 = 0.0F,
            .surface_water_kg_per_m2 = 0.8F,
        },
    };
    desc.forcing_rows = {
        mirakana::EnvironmentWeatherSimulationCellForcing{},
        mirakana::EnvironmentWeatherSimulationCellForcing{
            .surface_evaporation_kg_per_m2_s = 0.2F,
            .temperature_delta_celsius_per_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.5F,
        },
        mirakana::EnvironmentWeatherSimulationCellForcing{
            .surface_evaporation_kg_per_m2_s = 0.1F,
            .temperature_delta_celsius_per_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.25F,
        },
        mirakana::EnvironmentWeatherSimulationCellForcing{
            .surface_evaporation_kg_per_m2_s = 0.05F,
            .temperature_delta_celsius_per_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.0F,
        },
    };
    return desc;
}

[[nodiscard]] EnvironmentWeatherSimulationPackageEvidence
build_environment_weather_simulation_package_evidence(const DesktopRuntimeGameOptions& options) {
    EnvironmentWeatherSimulationPackageEvidence evidence;
    evidence.requested = options.require_environment_weather_simulation_package;
    if (!evidence.requested) {
        return evidence;
    }

    evidence.plan =
        mirakana::simulate_environment_weather_cpu_reference(make_environment_weather_simulation_package_desc());
    evidence.step_count = evidence.plan.succeeded() ? 1U : 0U;
    return evidence;
}

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_scene_vertex_buffers() {
    const auto layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(
        mirakana::runtime::RuntimeMeshPayload{.has_normals = true, .has_uvs = true, .has_tangent_frame = true});
    return layout.vertex_buffers;
}

[[nodiscard]] std::vector<mirakana::rhi::VertexAttributeDesc> runtime_scene_vertex_attributes() {
    const auto layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(
        mirakana::runtime::RuntimeMeshPayload{.has_normals = true, .has_uvs = true, .has_tangent_frame = true});
    return layout.vertex_attributes;
}

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_skinned_scene_vertex_buffers() {
    const auto layout = mirakana::runtime_rhi::make_runtime_skinned_mesh_vertex_layout_desc(
        mirakana::runtime::RuntimeSkinnedMeshPayload{});
    return layout.vertex_buffers;
}

[[nodiscard]] std::vector<mirakana::rhi::VertexAttributeDesc> runtime_skinned_scene_vertex_attributes() {
    const auto layout = mirakana::runtime_rhi::make_runtime_skinned_mesh_vertex_layout_desc(
        mirakana::runtime::RuntimeSkinnedMeshPayload{});
    return layout.vertex_attributes;
}

class SampleDesktopRuntimeGame final : public mirakana::GameApp {
  public:
    SampleDesktopRuntimeGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                             std::optional<mirakana::RuntimeSceneRenderInstance> scene, bool scene_gpu_mode,
                             std::vector<mirakana::AnimationJointTrack3dDesc> quaternion_animation_tracks,
                             bool lighting_shadow_policy_mode, bool textured_ui_atlas_mode,
                             mirakana::UiRendererImagePalette image_palette, std::uint32_t scene_mesh_instance_count,
                             mirakana::Win32DesktopPresentation* presentation = nullptr)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)),
          quaternion_animation_tracks_(std::move(quaternion_animation_tracks)),
          image_palette_(std::move(image_palette)), scene_gpu_mode_(scene_gpu_mode),
          lighting_shadow_policy_mode_(lighting_shadow_policy_mode), textured_ui_atlas_mode_(textured_ui_atlas_mode),
          scene_mesh_instance_count_(scene_mesh_instance_count), presentation_(presentation) {}

    void on_start(mirakana::EngineContext&) override {
        input_.press(mirakana::Key::right);
        ui_ok_ = build_hud();
        theme_.add(mirakana::UiThemeColor{.token = "hud.panel",
                                          .color = mirakana::Color{.r = 0.06F, .g = 0.08F, .b = 0.09F, .a = 1.0F}});
        renderer_.set_clear_color(mirakana::Color{.r = 0.02F, .g = 0.03F, .b = 0.035F, .a = 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        renderer_.begin_frame();

        const auto axis =
            input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
        if (!scene_gpu_mode_) {
            transform_.position = transform_.position + axis;
            renderer_.draw_sprite(mirakana::SpriteCommand{
                .transform = transform_, .color = mirakana::Color{.r = 0.8F, .g = 0.35F, .b = 0.15F, .a = 1.0F}});
        }
        if (scene_.has_value()) {
            std::optional<mirakana::SceneRenderPacket> rebuilt_packet;
            const auto* render_packet = &scene_->render_packet;
            if (scene_->scene.has_value()) {
                if (auto* camera = scene_->scene->find_node(kPrimaryCameraNode);
                    camera != nullptr && camera->components.camera.has_value() && camera->components.camera->primary) {
                    camera->transform.position.x += axis.x;
                    final_camera_x_ = camera->transform.position.x;
                    ++camera_controller_ticks_;
                }
                if (!quaternion_animation_tracks_.empty()) {
                    const auto apply_result = mirakana::sample_and_apply_runtime_scene_render_animation_pose_3d(
                        *scene_, packaged_quaternion_animation_skeleton(), quaternion_animation_tracks_, 1.0F);
                    quaternion_animation_tracks_sampled_ += apply_result.sampled_track_count;
                    if (apply_result.succeeded) {
                        const auto pose = mirakana::sample_animation_local_pose_3d(
                            packaged_quaternion_animation_skeleton(), quaternion_animation_tracks_, 1.0F);
                        const auto* animated = scene_->scene->find_node(kPackagedMeshNode);
                        quaternion_animation_scene_applied_ +=
                            static_cast<std::uint32_t>(apply_result.applied_sample_count);
                        if (pose.joints.size() == 1U && animated != nullptr) {
                            ++quaternion_animation_ticks_;
                            quaternion_animation_seen_ = true;
                            final_quaternion_z_ = pose.joints[0].rotation.z;
                            final_quaternion_w_ = pose.joints[0].rotation.w;
                            final_quaternion_scene_rotation_z_ = animated->transform.rotation_radians.z;
                        } else {
                            ++quaternion_animation_failures_;
                        }
                    } else {
                        ++quaternion_animation_failures_;
                    }
                }
                rebuilt_packet = mirakana::build_scene_render_packet(*scene_->scene);
                render_packet = &*rebuilt_packet;
            }
            const auto mesh_plan = mirakana::plan_scene_mesh_draws(*render_packet);
            scene_mesh_plan_ok_ = scene_mesh_plan_ok_ && mesh_plan.succeeded();
            scene_mesh_plan_meshes_ += mesh_plan.mesh_count;
            scene_mesh_plan_draws_ += mesh_plan.draw_count;
            scene_mesh_plan_unique_meshes_ += mesh_plan.unique_mesh_count;
            scene_mesh_plan_unique_materials_ += mesh_plan.unique_material_count;
            scene_mesh_plan_diagnostics_ += mesh_plan.diagnostics.size();
            if (lighting_shadow_policy_mode_) {
                const auto lighting_policy = mirakana::plan_scene_lighting_shadow_policy(
                    *render_packet, mirakana::SceneLightingShadowPolicyDesc{
                                        .max_light_count = 8,
                                        .max_shadowed_light_count = 1,
                                        .shadow_map =
                                            mirakana::SceneShadowMapDesc{
                                                .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
                                                .depth_format = mirakana::rhi::Format::depth24_stencil8,
                                                .directional_cascade_count = 1,
                                            },
                                    });
                lighting_shadow_policy_ok_ = lighting_shadow_policy_ok_ && lighting_policy.succeeded();
                lighting_shadow_policy_light_count_ = lighting_policy.light_count;
                lighting_shadow_policy_directional_light_count_ = lighting_policy.directional_light_count;
                lighting_shadow_policy_point_light_count_ = lighting_policy.point_light_count;
                lighting_shadow_policy_spot_light_count_ = lighting_policy.spot_light_count;
                lighting_shadow_policy_shadowed_light_count_ = lighting_policy.shadowed_light_count;
                lighting_shadow_policy_directional_cascade_count_ = lighting_policy.directional_cascade_count;
                lighting_shadow_policy_shadow_atlas_width_ = lighting_policy.shadow_atlas_extent.width;
                lighting_shadow_policy_shadow_atlas_height_ = lighting_policy.shadow_atlas_extent.height;
                lighting_shadow_policy_light_rows_ = lighting_policy.light_rows.size();
                lighting_shadow_policy_diagnostics_ += lighting_policy.diagnostics.size();
                if (lighting_policy.succeeded() && lighting_policy.light_count > 0 &&
                    lighting_policy.shadowed_light_count > 0 &&
                    lighting_policy.light_rows.size() == static_cast<std::size_t>(lighting_policy.light_count) &&
                    lighting_policy.directional_cascade_count > 0 && lighting_policy.shadow_atlas_extent.width > 0 &&
                    lighting_policy.shadow_atlas_extent.height > 0) {
                    ++lighting_shadow_policy_ready_frames_;
                }
            }
            mirakana::ScenePbrGpuSubmitContext pbr_gpu{};
            const mirakana::ScenePbrGpuSubmitContext* pbr_ptr = nullptr;
            if (scene_gpu_mode_ && presentation_ != nullptr) {
                auto* device = presentation_->scene_pbr_frame_rhi_device();
                const auto scene_ubo = presentation_->scene_pbr_frame_uniform_buffer();
                if (device != nullptr && scene_ubo.value != 0) {
                    const float aspect = renderer_.backbuffer_extent().height > 0
                                             ? static_cast<float>(renderer_.backbuffer_extent().width) /
                                                   static_cast<float>(renderer_.backbuffer_extent().height)
                                             : (16.0F / 9.0F);
                    mirakana::SceneCameraMatrices camera{};
                    mirakana::Vec3 cam_pos{.x = 0.0F, .y = 0.0F, .z = 5.0F};
                    if (const auto* primary = render_packet->primary_camera(); primary != nullptr) {
                        camera = mirakana::make_scene_camera_matrices(*primary, aspect);
                        cam_pos = mirakana::Vec3{.x = primary->world_from_node.at(0, 3),
                                                 .y = primary->world_from_node.at(1, 3),
                                                 .z = primary->world_from_node.at(2, 3)};
                    }
                    pbr_gpu.device = device;
                    pbr_gpu.scene_frame_uniform = scene_ubo;
                    pbr_gpu.camera = camera;
                    pbr_gpu.camera_world_position = cam_pos;
                    pbr_gpu.viewport_aspect = aspect;
                    pbr_ptr = &pbr_gpu;
                }
            }
            const auto scene_submit = mirakana::submit_scene_render_packet(
                renderer_, *render_packet,
                mirakana::SceneRenderSubmitDesc{
                    .fallback_mesh_color = mirakana::Color{.r = 0.8F, .g = 0.35F, .b = 0.15F, .a = 1.0F},
                    .material_palette = &scene_->material_palette,
                    .pbr_gpu = pbr_ptr,
                    .mesh_instance_count = scene_mesh_instance_count_,
                });
            scene_meshes_submitted_ += scene_submit.meshes_submitted;
            scene_materials_resolved_ += scene_submit.material_colors_resolved;
            primary_camera_seen_ = primary_camera_seen_ || scene_submit.has_primary_camera;
        }

        update_hud_text();
        const auto layout =
            mirakana::ui::solve_layout(hud_, mirakana::ui::ElementId{"hud.root"},
                                       mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 320.0F, .height = 180.0F});
        const auto submission = mirakana::ui::build_renderer_submission(hud_, layout);
        mirakana::UiRenderSubmitDesc ui_submit_desc;
        ui_submit_desc.theme = &theme_;
        if (textured_ui_atlas_mode_) {
            ui_submit_desc.image_palette = &image_palette_;
        }
        const auto hud_submit = mirakana::submit_ui_renderer_submission(renderer_, submission, ui_submit_desc);
        hud_boxes_submitted_ += hud_submit.boxes_submitted;
        hud_images_submitted_ += hud_submit.image_sprites_submitted;

        renderer_.end_frame();
        ++frames_;
        input_.begin_frame();

        if (throttle_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        return !input_.key_down(mirakana::Key::escape);
    }

    [[nodiscard]] bool hud_passed(std::uint32_t expected_frames) const noexcept {
        const bool image_rows_ok = !textured_ui_atlas_mode_ || hud_images_submitted_ == expected_frames;
        return ui_ok_ && ui_text_updates_ok_ && hud_boxes_submitted_ == expected_frames && image_rows_ok;
    }

    [[nodiscard]] bool packaged_scene_passed(std::uint32_t expected_frames) const noexcept {
        return primary_camera_seen_ && camera_controller_ticks_ == expected_frames &&
               final_camera_x_ == static_cast<float>(expected_frames) &&
               scene_meshes_submitted_ == static_cast<std::size_t>(expected_frames) &&
               scene_materials_resolved_ == static_cast<std::size_t>(expected_frames) && scene_mesh_plan_ok_ &&
               scene_mesh_plan_meshes_ == static_cast<std::uint64_t>(expected_frames) &&
               scene_mesh_plan_draws_ == static_cast<std::uint64_t>(expected_frames) &&
               scene_mesh_plan_unique_meshes_ == static_cast<std::uint64_t>(expected_frames) &&
               scene_mesh_plan_unique_materials_ == static_cast<std::uint64_t>(expected_frames) &&
               scene_mesh_plan_diagnostics_ == 0;
    }

    [[nodiscard]] bool quaternion_animation_passed(std::uint32_t expected_frames) const noexcept {
        return quaternion_animation_seen_ && quaternion_animation_ticks_ == expected_frames &&
               quaternion_animation_tracks_sampled_ ==
                   static_cast<std::uint64_t>(expected_frames) * quaternion_animation_tracks_.size() &&
               quaternion_animation_scene_applied_ == expected_frames && quaternion_animation_failures_ == 0 &&
               std::abs(final_quaternion_z_ - 1.0F) < 0.0001F && std::abs(final_quaternion_w_) < 0.0001F &&
               std::abs(final_quaternion_scene_rotation_z_ - 3.14159265F) < 0.0001F;
    }

    [[nodiscard]] bool lighting_shadow_policy_passed(std::uint32_t expected_frames) const noexcept {
        return lighting_shadow_policy_ok_ && lighting_shadow_policy_ready_frames_ == expected_frames &&
               lighting_shadow_policy_diagnostics_ == 0 && lighting_shadow_policy_light_count_ > 0 &&
               lighting_shadow_policy_shadowed_light_count_ > 0 &&
               lighting_shadow_policy_light_rows_ == static_cast<std::size_t>(lighting_shadow_policy_light_count_) &&
               lighting_shadow_policy_directional_cascade_count_ > 0 &&
               lighting_shadow_policy_shadow_atlas_width_ > 0 && lighting_shadow_policy_shadow_atlas_height_ > 0;
    }

    [[nodiscard]] std::uint32_t frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] std::size_t scene_meshes_submitted() const noexcept {
        return scene_meshes_submitted_;
    }

    [[nodiscard]] std::size_t scene_materials_resolved() const noexcept {
        return scene_materials_resolved_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_meshes() const noexcept {
        return scene_mesh_plan_meshes_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_draws() const noexcept {
        return scene_mesh_plan_draws_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_unique_meshes() const noexcept {
        return scene_mesh_plan_unique_meshes_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_unique_materials() const noexcept {
        return scene_mesh_plan_unique_materials_;
    }

    [[nodiscard]] std::size_t scene_mesh_plan_diagnostics() const noexcept {
        return scene_mesh_plan_diagnostics_;
    }

    [[nodiscard]] bool primary_camera_seen() const noexcept {
        return primary_camera_seen_;
    }

    [[nodiscard]] std::uint32_t camera_controller_ticks() const noexcept {
        return camera_controller_ticks_;
    }

    [[nodiscard]] float final_camera_x() const noexcept {
        return final_camera_x_;
    }

    [[nodiscard]] std::size_t hud_boxes_submitted() const noexcept {
        return hud_boxes_submitted_;
    }

    [[nodiscard]] std::size_t hud_images_submitted() const noexcept {
        return hud_images_submitted_;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_ticks() const noexcept {
        return quaternion_animation_ticks_;
    }

    [[nodiscard]] std::uint64_t quaternion_animation_tracks_sampled() const noexcept {
        return quaternion_animation_tracks_sampled_;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_failures() const noexcept {
        return quaternion_animation_failures_;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_scene_applied() const noexcept {
        return quaternion_animation_scene_applied_;
    }

    [[nodiscard]] float final_quaternion_z() const noexcept {
        return final_quaternion_z_;
    }

    [[nodiscard]] float final_quaternion_w() const noexcept {
        return final_quaternion_w_;
    }

    [[nodiscard]] float final_quaternion_scene_rotation_z() const noexcept {
        return final_quaternion_scene_rotation_z_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_light_count() const noexcept {
        return lighting_shadow_policy_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_directional_light_count() const noexcept {
        return lighting_shadow_policy_directional_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_point_light_count() const noexcept {
        return lighting_shadow_policy_point_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_spot_light_count() const noexcept {
        return lighting_shadow_policy_spot_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_shadowed_light_count() const noexcept {
        return lighting_shadow_policy_shadowed_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_directional_cascade_count() const noexcept {
        return lighting_shadow_policy_directional_cascade_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_shadow_atlas_width() const noexcept {
        return lighting_shadow_policy_shadow_atlas_width_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_shadow_atlas_height() const noexcept {
        return lighting_shadow_policy_shadow_atlas_height_;
    }

    [[nodiscard]] std::size_t lighting_shadow_policy_light_rows() const noexcept {
        return lighting_shadow_policy_light_rows_;
    }

    [[nodiscard]] std::size_t lighting_shadow_policy_diagnostics() const noexcept {
        return lighting_shadow_policy_diagnostics_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_ready_frames() const noexcept {
        return lighting_shadow_policy_ready_frames_;
    }

  private:
    [[nodiscard]] bool build_hud() {
        mirakana::ui::ElementDesc root;
        root.id = mirakana::ui::ElementId{"hud.root"};
        root.role = mirakana::ui::SemanticRole::root;
        root.style.layout = mirakana::ui::LayoutMode::column;
        root.style.padding = mirakana::ui::EdgeInsets{.top = 8.0F, .right = 8.0F, .bottom = 8.0F, .left = 8.0F};
        root.style.gap = 4.0F;
        if (!hud_.try_add_element(root)) {
            return false;
        }

        mirakana::ui::ElementDesc status;
        status.id = mirakana::ui::ElementId{"hud.status"};
        status.parent = root.id;
        status.role = mirakana::ui::SemanticRole::label;
        status.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 144.0F, .height = 24.0F};
        status.text = mirakana::ui::TextContent{
            .label = "3D Meshes 0", .localization_key = "hud.status", .font_family = "engine-default"};
        status.style.background_token = "hud.panel";
        status.accessibility_label = "3D diagnostics";
        if (!hud_.try_add_element(status)) {
            return false;
        }
        if (!textured_ui_atlas_mode_) {
            return true;
        }

        mirakana::ui::ElementDesc atlas_image;
        atlas_image.id = mirakana::ui::ElementId{"hud.texture_atlas_proof"};
        atlas_image.parent = root.id;
        atlas_image.role = mirakana::ui::SemanticRole::image;
        atlas_image.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 32.0F};
        atlas_image.image.resource_id = std::string{kHudAtlasProofResourceId};
        atlas_image.image.asset_uri = std::string{kHudAtlasProofAssetUri};
        atlas_image.accessibility_label = "Texture atlas proof";
        return hud_.try_add_element(atlas_image);
    }

    void update_hud_text() {
        const auto text = std::string{"3D Meshes "} + std::to_string(scene_meshes_submitted_);
        ui_text_updates_ok_ =
            ui_text_updates_ok_ &&
            hud_.set_text(mirakana::ui::ElementId{"hud.status"},
                          mirakana::ui::TextContent{
                              .label = text, .localization_key = "hud.status", .font_family = "engine-default"});
    }

    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::Transform2D transform_;
    mirakana::ui::UiDocument hud_;
    mirakana::UiRendererTheme theme_;
    mirakana::UiRendererImagePalette image_palette_;
    bool throttle_{true};
    std::optional<mirakana::RuntimeSceneRenderInstance> scene_;
    std::vector<mirakana::AnimationJointTrack3dDesc> quaternion_animation_tracks_;
    bool scene_gpu_mode_{false};
    bool lighting_shadow_policy_mode_{false};
    bool textured_ui_atlas_mode_{false};
    std::uint32_t scene_mesh_instance_count_{1};
    mirakana::Win32DesktopPresentation* presentation_{nullptr};
    std::uint32_t frames_{0};
    std::size_t scene_meshes_submitted_{0};
    std::size_t scene_materials_resolved_{0};
    std::uint64_t scene_mesh_plan_meshes_{0};
    std::uint64_t scene_mesh_plan_draws_{0};
    std::uint64_t scene_mesh_plan_unique_meshes_{0};
    std::uint64_t scene_mesh_plan_unique_materials_{0};
    std::size_t scene_mesh_plan_diagnostics_{0};
    std::size_t hud_boxes_submitted_{0};
    std::size_t hud_images_submitted_{0};
    std::uint32_t camera_controller_ticks_{0};
    std::uint32_t quaternion_animation_ticks_{0};
    std::uint64_t quaternion_animation_tracks_sampled_{0};
    std::uint32_t quaternion_animation_failures_{0};
    std::uint32_t quaternion_animation_scene_applied_{0};
    std::uint32_t lighting_shadow_policy_ready_frames_{0};
    std::uint32_t lighting_shadow_policy_light_count_{0};
    std::uint32_t lighting_shadow_policy_directional_light_count_{0};
    std::uint32_t lighting_shadow_policy_point_light_count_{0};
    std::uint32_t lighting_shadow_policy_spot_light_count_{0};
    std::uint32_t lighting_shadow_policy_shadowed_light_count_{0};
    std::uint32_t lighting_shadow_policy_directional_cascade_count_{0};
    std::uint32_t lighting_shadow_policy_shadow_atlas_width_{0};
    std::uint32_t lighting_shadow_policy_shadow_atlas_height_{0};
    std::size_t lighting_shadow_policy_light_rows_{0};
    std::size_t lighting_shadow_policy_diagnostics_{0};
    float final_camera_x_{0.0F};
    float final_quaternion_z_{0.0F};
    float final_quaternion_w_{1.0F};
    float final_quaternion_scene_rotation_z_{0.0F};
    bool ui_ok_{false};
    bool ui_text_updates_ok_{true};
    bool primary_camera_seen_{false};
    bool quaternion_animation_seen_{false};
    bool scene_mesh_plan_ok_{true};
    bool lighting_shadow_policy_ok_{true};
};

[[nodiscard]] bool parse_positive_uint32(std::string_view text, std::uint32_t& value) noexcept {
    std::uint32_t parsed{};
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0) {
        return false;
    }
    value = parsed;
    return true;
}

void enable_environment_ready_aggregate_requirements(DesktopRuntimeGameOptions& options) noexcept {
    options.require_environment_ready_aggregate = true;
    options.require_environment_profile = true;
    options.require_d3d12_scene_shaders = true;
    options.require_d3d12_renderer = true;
    options.require_scene_gpu_bindings = true;
    options.require_postprocess = true;
    options.require_postprocess_depth_input = true;
    options.require_d3d12_postprocess_evidence = true;
    options.require_environment_fog_evidence = true;
    options.require_physical_sky_package_evidence = true;
    options.require_environment_lighting_package_evidence = true;
    options.require_environment_lighting_renderer_execution = true;
    options.require_cloud_layer_package_evidence = true;
    options.require_cloud_layer_renderer_execution = true;
    options.require_environment_precipitation_package_evidence = true;
    options.require_environment_precipitation_renderer_execution = true;
    options.require_environment_volumetric_fog_package_evidence = true;
    options.require_environment_volumetric_cloud_package_evidence = true;
    options.require_environment_volumetric_cloud_renderer_execution = true;
    options.require_environment_material_weathering = true;
    options.require_environment_audio_playback = true;
}

void enable_environment_vulkan_strict_aggregate_requirements(DesktopRuntimeGameOptions& options) noexcept {
    options.require_environment_vulkan_strict_aggregate = true;
    options.require_environment_profile = true;
    options.require_vulkan_scene_shaders = true;
    options.require_vulkan_renderer = true;
    options.require_scene_gpu_bindings = true;
    options.require_postprocess = true;
    options.require_postprocess_depth_input = true;
    options.require_vulkan_postprocess_evidence = true;
    options.require_physical_sky_vulkan_package_evidence = true;
    options.require_environment_fog_vulkan_package_evidence = true;
    options.require_environment_lighting_vulkan_renderer_execution = true;
    options.require_environment_volumetric_fog_vulkan_renderer_execution = true;
    options.require_environment_volumetric_cloud_vulkan_renderer_execution = true;
    options.require_environment_precipitation_vulkan_renderer_execution = true;
}

void enable_environment_backend_parity_requirements(DesktopRuntimeGameOptions& options) noexcept {
    options.require_environment_backend_parity = true;
    enable_environment_ready_aggregate_requirements(options);
}

void enable_environment_platform_readiness_requirements(DesktopRuntimeGameOptions& options) noexcept {
    options.require_environment_platform_readiness = true;
    enable_environment_ready_aggregate_requirements(options);
}

void enable_environment_optimization_measurement_requirements(DesktopRuntimeGameOptions& options) noexcept {
    options.require_environment_optimization_measurement = true;
    enable_environment_ready_aggregate_requirements(options);
}

void enable_environment_weather_simulation_package_requirements(DesktopRuntimeGameOptions& options) noexcept {
    options.require_environment_weather_simulation_package = true;
}

void print_usage() {
    std::cout << "sample_desktop_runtime_game [--smoke] [--max-frames N] "
                 "[--require-config PATH] [--require-scene-package PATH] [--require-d3d12-scene-shaders] "
                 "[--require-vulkan-scene-shaders] [--require-d3d12-renderer] [--require-vulkan-renderer] "
                 "[--require-scene-gpu-bindings] [--require-postprocess] [--require-postprocess-depth-input] "
                 "[--require-directional-shadow] [--require-directional-shadow-filtering] "
                 "[--require-d3d12-shadow-cascade-policy] [--require-vulkan-shadow-cascade-policy] "
                 "[--require-lighting-shadow-policy] "
                 "[--require-scene-scale-policy] [--require-d3d12-instanced-draw-evidence] "
                 "[--require-vulkan-instanced-draw-evidence] "
                 "[--require-d3d12-postprocess-evidence] "
                 "[--require-vulkan-postprocess-evidence] "
                 "[--require-environment-profile] "
                 "[--require-environment-fog-evidence] "
                 "[--require-environment-fog-vulkan-package-evidence] "
                 "[--require-physical-sky-package-evidence] "
                 "[--require-environment-volumetric-fog-package-evidence] "
                 "[--require-environment-lighting-package-evidence] "
                 "[--require-environment-lighting-renderer-execution] "
                 "[--require-environment-lighting-vulkan-renderer-execution] "
                 "[--require-cloud-layer-package-evidence] [--require-cloud-layer-renderer-execution] "
                 "[--require-environment-precipitation-package-evidence] "
                 "[--require-environment-precipitation-renderer-execution] "
                 "[--require-environment-precipitation-vulkan-renderer-execution] "
                 "[--require-environment-snow-package-evidence] "
                 "[--require-environment-snow-renderer-execution] "
                 "[--require-volumetric-cloud-package-evidence] "
                 "[--require-volumetric-cloud-renderer-execution] "
                 "[--require-environment-volumetric-cloud-vulkan-renderer-execution] "
                 "[--require-environment-material-weathering] "
                 "[--require-environment-audio-playback] "
                 "[--require-environment-texture-asset-pipeline-package] "
                 "[--require-environment-preset-library-package] "
                 "[--require-environment-ready-aggregate] "
                 "[--require-environment-vulkan-strict-aggregate] "
                 "[--require-environment-backend-parity] "
                 "[--require-environment-platform-readiness] "
                 "[--require-environment-optimization-measurement] "
                 "[--require-environment-weather-simulation-package] "
                 "[--require-gpu-memory-policy] [--require-memory-diagnostics] [--require-d3d12-gpu-memory-evidence] "
                 "[--require-vulkan-gpu-memory-evidence] "
                 "[--require-debug-profiling-policy] [--require-d3d12-debug-profiling-evidence] "
                 "[--require-vulkan-debug-profiling-evidence] "
                 "[--require-job-scheduling-evidence] [--require-job-execution-foundation] "
                 "[--require-job-execution-topology-policy] [--require-job-execution-work-stealing] "
                 "[--require-job-execution-placement-policy] [--require-windows-cpu-set-worker-placement] "
                 "[--require-simd-dispatch-policy] "
                 "[--require-renderer-quality-gates] "
                 "[--require-framegraph-multiqueue-evidence] "
                 "[--require-native-ui-overlay] "
                 "[--require-native-ui-textured-sprite-atlas] "
                 "[--require-gpu-skinning] [--require-d3d12-gpu-skinning-evidence] "
                 "[--require-vulkan-gpu-skinning-evidence] "
                 "[--require-quaternion-animation]\n";
}

[[nodiscard]] bool parse_args(int argc, char** argv, DesktopRuntimeGameOptions& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg{argv[index]};
        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            return true;
        }
        if (arg == "--smoke") {
            options.smoke = true;
            continue;
        }
        if (arg == "--require-d3d12-scene-shaders") {
            options.require_d3d12_scene_shaders = true;
            continue;
        }
        if (arg == "--require-vulkan-scene-shaders") {
            options.require_vulkan_scene_shaders = true;
            continue;
        }
        if (arg == "--require-d3d12-renderer") {
            options.require_d3d12_renderer = true;
            continue;
        }
        if (arg == "--require-vulkan-renderer") {
            options.require_vulkan_renderer = true;
            continue;
        }
        if (arg == "--require-scene-gpu-bindings") {
            options.require_scene_gpu_bindings = true;
            continue;
        }
        if (arg == "--require-postprocess") {
            options.require_postprocess = true;
            continue;
        }
        if (arg == "--require-postprocess-depth-input") {
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            continue;
        }
        if (arg == "--require-directional-shadow") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            continue;
        }
        if (arg == "--require-directional-shadow-filtering") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            continue;
        }
        if (arg == "--require-d3d12-shadow-cascade-policy") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_d3d12_shadow_cascade_policy = true;
            continue;
        }
        if (arg == "--require-vulkan-shadow-cascade-policy") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_vulkan_shadow_cascade_policy = true;
            continue;
        }
        if (arg == "--require-lighting-shadow-policy") {
            options.require_lighting_shadow_policy = true;
            continue;
        }
        if (arg == "--require-scene-scale-policy") {
            options.require_scene_gpu_bindings = true;
            options.require_scene_scale_policy = true;
            continue;
        }
        if (arg == "--require-d3d12-instanced-draw-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_scene_scale_policy = true;
            options.require_d3d12_instanced_draw_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-instanced-draw-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_scene_scale_policy = true;
            options.require_vulkan_instanced_draw_evidence = true;
            continue;
        }
        if (arg == "--require-d3d12-postprocess-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_d3d12_postprocess_evidence = true;
            continue;
        }
        if (arg == "--require-environment-profile") {
            options.require_environment_profile = true;
            continue;
        }
        if (arg == "--require-environment-fog-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_fog_evidence = true;
            continue;
        }
        if (arg == "--require-environment-fog-vulkan-package-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_vulkan_postprocess_evidence = true;
            options.require_environment_fog_vulkan_package_evidence = true;
            continue;
        }
        if (arg == "--require-environment-volumetric-fog-package-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_environment_volumetric_fog_package_evidence = true;
            continue;
        }
        if (arg == "--require-environment-volumetric-fog-vulkan-renderer-execution") {
            options.require_vulkan_scene_shaders = true;
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_vulkan_postprocess_evidence = true;
            options.require_environment_volumetric_fog_vulkan_renderer_execution = true;
            continue;
        }
        if (arg == "--require-physical-sky-package-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_physical_sky_package_evidence = true;
            continue;
        }
        if (arg == "--require-physical-sky-vulkan-package-evidence") {
            options.require_vulkan_scene_shaders = true;
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_physical_sky_vulkan_package_evidence = true;
            continue;
        }
        if (arg == "--require-environment-lighting-package-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_lighting_package_evidence = true;
            continue;
        }
        if (arg == "--require-environment-lighting-renderer-execution") {
            options.require_d3d12_scene_shaders = true;
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_lighting_package_evidence = true;
            options.require_environment_lighting_renderer_execution = true;
            continue;
        }
        if (arg == "--require-environment-lighting-vulkan-renderer-execution") {
            options.require_vulkan_scene_shaders = true;
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_vulkan_postprocess_evidence = true;
            options.require_environment_lighting_vulkan_renderer_execution = true;
            continue;
        }
        if (arg == "--require-cloud-layer-package-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_cloud_layer_package_evidence = true;
            continue;
        }
        if (arg == "--require-cloud-layer-renderer-execution") {
            options.require_d3d12_scene_shaders = true;
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_cloud_layer_package_evidence = true;
            options.require_cloud_layer_renderer_execution = true;
            continue;
        }
        if (arg == "--require-environment-precipitation-package-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_precipitation_package_evidence = true;
            continue;
        }
        if (arg == "--require-environment-precipitation-renderer-execution") {
            options.require_d3d12_scene_shaders = true;
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_precipitation_package_evidence = true;
            options.require_environment_precipitation_renderer_execution = true;
            continue;
        }
        if (arg == "--require-environment-precipitation-vulkan-renderer-execution") {
            options.require_vulkan_scene_shaders = true;
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_vulkan_postprocess_evidence = true;
            options.require_environment_precipitation_vulkan_renderer_execution = true;
            continue;
        }
        if (arg == "--require-environment-snow-package-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_snow_package_evidence = true;
            continue;
        }
        if (arg == "--require-environment-snow-renderer-execution") {
            options.require_d3d12_scene_shaders = true;
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_snow_package_evidence = true;
            options.require_environment_snow_renderer_execution = true;
            continue;
        }
        if (arg == "--require-volumetric-cloud-package-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_volumetric_cloud_package_evidence = true;
            continue;
        }
        if (arg == "--require-volumetric-cloud-renderer-execution") {
            options.require_d3d12_scene_shaders = true;
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_volumetric_cloud_package_evidence = true;
            options.require_environment_volumetric_cloud_renderer_execution = true;
            continue;
        }
        if (arg == "--require-environment-volumetric-cloud-vulkan-renderer-execution") {
            options.require_vulkan_scene_shaders = true;
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_vulkan_postprocess_evidence = true;
            options.require_environment_volumetric_cloud_vulkan_renderer_execution = true;
            continue;
        }
        if (arg == "--require-environment-material-weathering") {
            options.require_d3d12_scene_shaders = true;
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_material_weathering = true;
            continue;
        }
        if (arg == "--require-environment-audio-playback") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_d3d12_postprocess_evidence = true;
            options.require_environment_precipitation_package_evidence = true;
            options.require_environment_audio_playback = true;
            continue;
        }
        if (arg == "--require-environment-texture-asset-pipeline-package") {
            options.require_environment_profile = true;
            options.require_environment_texture_asset_pipeline_package = true;
            continue;
        }
        if (arg == "--require-environment-preset-library-package") {
            options.require_environment_profile = true;
            options.require_environment_preset_library_package = true;
            continue;
        }
        if (arg == "--require-environment-ready-aggregate") {
            enable_environment_ready_aggregate_requirements(options);
            continue;
        }
        if (arg == "--require-environment-vulkan-strict-aggregate") {
            enable_environment_vulkan_strict_aggregate_requirements(options);
            continue;
        }
        if (arg == "--require-environment-backend-parity") {
            enable_environment_backend_parity_requirements(options);
            continue;
        }
        if (arg == "--require-environment-platform-readiness") {
            enable_environment_platform_readiness_requirements(options);
            continue;
        }
        if (arg == "--require-environment-optimization-measurement") {
            enable_environment_optimization_measurement_requirements(options);
            continue;
        }
        if (arg == "--require-environment-weather-simulation-package") {
            enable_environment_weather_simulation_package_requirements(options);
            continue;
        }
        if (arg == "--require-vulkan-postprocess-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_vulkan_postprocess_evidence = true;
            continue;
        }
        if (arg == "--require-gpu-memory-policy") {
            options.require_scene_gpu_bindings = true;
            options.require_gpu_memory_policy = true;
            continue;
        }
        if (arg == "--require-memory-diagnostics") {
            options.require_scene_gpu_bindings = true;
            options.require_gpu_memory_policy = true;
            options.require_memory_diagnostics = true;
            continue;
        }
        if (arg == "--require-d3d12-gpu-memory-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_gpu_memory_policy = true;
            options.require_d3d12_gpu_memory_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-gpu-memory-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_gpu_memory_policy = true;
            options.require_vulkan_gpu_memory_evidence = true;
            continue;
        }
        if (arg == "--require-debug-profiling-policy") {
            options.require_scene_gpu_bindings = true;
            options.require_debug_profiling_policy = true;
            continue;
        }
        if (arg == "--require-d3d12-debug-profiling-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_debug_profiling_policy = true;
            options.require_d3d12_debug_profiling_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-debug-profiling-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_debug_profiling_policy = true;
            options.require_vulkan_debug_profiling_evidence = true;
            continue;
        }
        if (arg == "--require-job-scheduling-evidence") {
            options.require_job_scheduling_evidence = true;
            continue;
        }
        if (arg == "--require-job-execution-foundation") {
            options.require_job_execution_foundation = true;
            continue;
        }
        if (arg == "--require-job-execution-topology-policy") {
            options.require_job_execution_topology_policy = true;
            continue;
        }
        if (arg == "--require-job-execution-work-stealing") {
            options.require_job_execution_foundation = true;
            options.require_job_execution_topology_policy = true;
            options.require_job_execution_work_stealing = true;
            continue;
        }
        if (arg == "--require-job-execution-placement-policy") {
            options.require_job_execution_foundation = true;
            options.require_job_execution_topology_policy = true;
            options.require_job_execution_work_stealing = true;
            options.require_job_execution_placement_policy = true;
            continue;
        }
        if (arg == "--require-windows-cpu-set-worker-placement") {
            options.require_job_execution_foundation = true;
            options.require_job_execution_topology_policy = true;
            options.require_job_execution_work_stealing = true;
            options.require_job_execution_placement_policy = true;
            options.require_windows_cpu_set_worker_placement = true;
            continue;
        }
        if (arg == "--require-windows-cpu-set-smt-worker-placement") {
            options.require_job_execution_foundation = true;
            options.require_job_execution_topology_policy = true;
            options.require_job_execution_work_stealing = true;
            options.require_job_execution_placement_policy = true;
            options.require_windows_cpu_set_smt_worker_placement = true;
            continue;
        }
        if (arg == "--require-simd-dispatch-policy") {
            options.require_simd_dispatch_policy = true;
            continue;
        }
        if (arg == "--require-renderer-quality-gates") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_renderer_quality_gates = true;
            continue;
        }
        if (arg == "--require-framegraph-multiqueue-evidence") {
            options.require_framegraph_multiqueue_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-framegraph-multiqueue-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_vulkan_framegraph_multiqueue_evidence = true;
            continue;
        }
        if (arg == "--require-native-ui-overlay") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_native_ui_overlay = true;
            continue;
        }
        if (arg == "--require-native-ui-textured-sprite-atlas") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_native_ui_overlay = true;
            options.require_native_ui_textured_sprite_atlas = true;
            continue;
        }
        if (arg == "--require-gpu-skinning") {
            options.require_scene_gpu_bindings = true;
            options.require_gpu_skinning = true;
            continue;
        }
        if (arg == "--require-d3d12-gpu-skinning-evidence") {
            options.require_scene_gpu_bindings = true;
            options.require_gpu_skinning = true;
            options.require_d3d12_gpu_skinning_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-gpu-skinning-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_gpu_skinning = true;
            options.require_vulkan_gpu_skinning_evidence = true;
            continue;
        }
        if (arg == "--require-quaternion-animation") {
            options.require_quaternion_animation = true;
            continue;
        }
        if (arg == "--max-frames") {
            if (index + 1 >= argc || !parse_positive_uint32(argv[index + 1], options.max_frames)) {
                std::cerr << "--max-frames requires a positive integer\n";
                return false;
            }
            ++index;
            continue;
        }
        if (arg == "--require-config") {
            if (index + 1 >= argc) {
                std::cerr << "--require-config requires a relative path\n";
                return false;
            }
            options.required_config_path = argv[index + 1];
            ++index;
            continue;
        }
        if (arg == "--require-scene-package") {
            if (index + 1 >= argc) {
                std::cerr << "--require-scene-package requires a relative path\n";
                return false;
            }
            options.required_scene_package_path = argv[index + 1];
            ++index;
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return false;
    }

    if (options.require_d3d12_renderer && options.require_vulkan_renderer) {
        std::cerr << "--require-d3d12-renderer and --require-vulkan-renderer are mutually exclusive\n";
        return false;
    }

    if (options.smoke) {
        if (options.max_frames == 0) {
            options.max_frames = 2;
        }
        options.throttle = false;
    }
    return true;
}

[[nodiscard]] mirakana::Win32DesktopPresentationSceneScalePolicyDesc
make_scene_scale_policy_desc(const DesktopRuntimeGameOptions& options,
                             bool backend_instancing_evidence_ready) noexcept {
    mirakana::Win32DesktopPresentationSceneScalePolicyDesc desc;
    if (options.require_scene_scale_policy) {
        desc.require_scene_gpu_bindings = true;
        desc.expected_frames = options.max_frames;
        desc.require_backend_instancing_evidence =
            options.require_d3d12_instanced_draw_evidence || options.require_vulkan_instanced_draw_evidence;
        desc.backend_instancing_evidence_ready = backend_instancing_evidence_ready;
    }
    return desc;
}

[[nodiscard]] std::uint64_t expected_d3d12_instanced_draw_instances(const DesktopRuntimeGameOptions& options) noexcept {
    if (!options.require_d3d12_instanced_draw_evidence) {
        return 0;
    }
    return static_cast<std::uint64_t>(options.max_frames) * 3U;
}

[[nodiscard]] std::uint64_t
expected_vulkan_instanced_draw_instances(const DesktopRuntimeGameOptions& options) noexcept {
    if (!options.require_vulkan_instanced_draw_evidence) {
        return 0;
    }
    return static_cast<std::uint64_t>(options.max_frames) * 3U;
}

[[nodiscard]] std::uint32_t scene_mesh_instance_count(const DesktopRuntimeGameOptions& options) noexcept {
    return (options.require_d3d12_instanced_draw_evidence || options.require_vulkan_instanced_draw_evidence) ? 3U : 1U;
}

[[nodiscard]] bool
directional_shadow_cascade_policy_matches(const mirakana::Win32DesktopPresentationReport& report) noexcept {
    const bool atlas_matches =
        report.directional_shadow_cascade_tile_width > 0 &&
        report.directional_shadow_atlas_width ==
            report.directional_shadow_cascade_count * report.directional_shadow_cascade_tile_width &&
        report.directional_shadow_atlas_height == report.directional_shadow_cascade_tile_width;
    return report.directional_shadow_cascade_count == 4 && atlas_matches &&
           report.directional_shadow_light_space_cascades == report.directional_shadow_cascade_count &&
           report.directional_shadow_cascade_splits == report.directional_shadow_cascade_count + 1U;
}

[[nodiscard]] bool d3d12_shadow_cascade_policy_ready(const mirakana::Win32DesktopPresentationReport& report) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12 &&
           directional_shadow_cascade_policy_matches(report);
}

[[nodiscard]] bool vulkan_shadow_cascade_policy_ready(const mirakana::Win32DesktopPresentationReport& report) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan &&
           directional_shadow_cascade_policy_matches(report);
}

[[nodiscard]] bool
d3d12_framegraph_multiqueue_evidence_ready(const mirakana::Win32DesktopPresentationReport& report,
                                           const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12 && evidence.ready;
}

[[nodiscard]] bool
vulkan_framegraph_multiqueue_evidence_ready(const mirakana::Win32DesktopPresentationReport& report,
                                            const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan && evidence.ready;
}

[[nodiscard]] bool gpu_skinning_evidence_matches(const mirakana::Win32DesktopPresentationReport& report,
                                                 std::uint32_t max_frames) noexcept {
    const auto expected_frames = static_cast<std::uint64_t>(max_frames);
    return report.renderer_stats.gpu_skinning_draws == expected_frames &&
           report.renderer_stats.skinned_palette_descriptor_binds == expected_frames &&
           report.scene_gpu_stats.skinned_mesh_uploads >= 1 && report.scene_gpu_stats.skinned_mesh_bindings >= 1 &&
           report.scene_gpu_stats.skinned_mesh_bindings_resolved == static_cast<std::size_t>(max_frames);
}

[[nodiscard]] bool d3d12_gpu_skinning_evidence_ready(const mirakana::Win32DesktopPresentationReport& report,
                                                     std::uint32_t max_frames) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12 &&
           gpu_skinning_evidence_matches(report, max_frames);
}

[[nodiscard]] bool vulkan_gpu_skinning_evidence_ready(const mirakana::Win32DesktopPresentationReport& report,
                                                      std::uint32_t max_frames) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan &&
           gpu_skinning_evidence_matches(report, max_frames);
}

[[nodiscard]] mirakana::Win32DesktopPresentationGpuMemoryPolicyDesc
make_gpu_memory_policy_desc(const DesktopRuntimeGameOptions& options) noexcept {
    mirakana::Win32DesktopPresentationGpuMemoryPolicyDesc desc;
    if (options.require_gpu_memory_policy) {
        desc.require_scene_gpu_bindings = true;
        desc.expected_frames = options.max_frames;
        desc.require_backend_memory_evidence =
            options.require_d3d12_gpu_memory_evidence || options.require_vulkan_gpu_memory_evidence;
        desc.require_os_video_memory_budget = options.require_d3d12_gpu_memory_evidence;
        desc.require_declared_budget_evidence = true;
        desc.require_residency_pressure_evidence = true;
        desc.require_package_counter_evidence = true;
        desc.declared_local_budget_bytes = 64ULL * 1024ULL * 1024ULL;
        desc.residency_pressure_event_count = 1;
        desc.package_counter_evidence_ready = true;
    }
    return desc;
}

[[nodiscard]] mirakana::Win32DesktopPresentationDebugProfilingPolicyDesc
make_debug_profiling_policy_desc(const DesktopRuntimeGameOptions& options) noexcept {
    mirakana::Win32DesktopPresentationDebugProfilingPolicyDesc desc;
    if (options.require_debug_profiling_policy) {
        desc.require_scene_gpu_bindings = true;
        desc.expected_frames = options.max_frames;
        desc.require_backend_profiling_evidence =
            options.require_d3d12_debug_profiling_evidence || options.require_vulkan_debug_profiling_evidence;
        desc.require_cpu_profile_zone_evidence = true;
        desc.require_trace_capture_handoff_evidence = true;
        desc.require_package_counter_evidence = true;
        desc.cpu_profile_zone_count = 2;
        desc.trace_capture_handoff_row_count = 1;
        desc.package_counter_evidence_ready = true;
    }
    return desc;
}

[[nodiscard]] std::uint64_t positive_count_for_bytes(std::uint64_t bytes, std::uint64_t count) noexcept {
    return count > 0U ? count : static_cast<std::uint64_t>(bytes > 0U ? 1U : 0U);
}

[[nodiscard]] std::uint64_t
selected_gpu_memory_budget(const mirakana::Win32DesktopPresentationGpuMemoryPolicyReport& gpu_memory_policy) noexcept {
    if (gpu_memory_policy.os_local_budget_bytes > 0U) {
        return gpu_memory_policy.os_local_budget_bytes;
    }
    return 64ULL * 1024ULL * 1024ULL;
}

[[nodiscard]] mirakana::MemoryDiagnosticsSummary
summarize_package_memory_diagnostics(const std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                                     const mirakana::Win32DesktopPresentationGpuMemoryPolicyReport& gpu_memory_policy) {
    std::vector<mirakana::MemoryCounterRow> rows;

    if (runtime_package.has_value()) {
        const auto package_bytes = mirakana::runtime::estimate_runtime_asset_package_resident_bytes(*runtime_package);
        rows.push_back(mirakana::MemoryCounterRow{
            .lifetime_class = mirakana::MemoryLifetimeClass::package_resident_cpu,
            .name = "package.runtime_scene",
            .bytes = package_bytes,
            .allocation_count = static_cast<std::uint64_t>(runtime_package->records().size()),
            .high_water_bytes = package_bytes,
            .budget_bytes = 0U,
        });
    }

    const auto resident_gpu_bytes = gpu_memory_policy.committed_byte_estimate > 0U
                                        ? gpu_memory_policy.committed_byte_estimate
                                        : gpu_memory_policy.total_counted_bytes;
    if (resident_gpu_bytes > 0U) {
        rows.push_back(mirakana::MemoryCounterRow{
            .lifetime_class = mirakana::MemoryLifetimeClass::resident_gpu,
            .name = "rhi.committed_resources",
            .bytes = resident_gpu_bytes,
            .allocation_count = positive_count_for_bytes(resident_gpu_bytes, gpu_memory_policy.request_count),
            .high_water_bytes = resident_gpu_bytes,
            .budget_bytes = selected_gpu_memory_budget(gpu_memory_policy),
        });
    }

    if (gpu_memory_policy.upload_bytes_written > 0U) {
        rows.push_back(mirakana::MemoryCounterRow{
            .lifetime_class = mirakana::MemoryLifetimeClass::upload_staging,
            .name = "rhi.upload_staging",
            .bytes = gpu_memory_policy.upload_bytes_written,
            .allocation_count = positive_count_for_bytes(gpu_memory_policy.upload_bytes_written,
                                                         gpu_memory_policy.upload_pressure_request_count),
            .high_water_bytes = gpu_memory_policy.upload_bytes_written,
            .budget_bytes = 0U,
        });
    }

    const auto transient_allocations = gpu_memory_policy.transient_placed_allocations > 0U
                                           ? gpu_memory_policy.transient_placed_allocations
                                           : gpu_memory_policy.transient_heap_allocations;
    if (transient_allocations > 0U || gpu_memory_policy.transient_placed_resources_alive > 0U) {
        rows.push_back(mirakana::MemoryCounterRow{
            .lifetime_class = mirakana::MemoryLifetimeClass::transient_gpu,
            .name = "rhi.transient_textures",
            .bytes = 0U,
            .allocation_count = transient_allocations,
            .high_water_bytes = 0U,
            .budget_bytes = 0U,
        });
    }

    return mirakana::summarize_memory_diagnostics(
        rows, mirakana::MemoryDiagnosticsOptions{.budget_pressure_warning_ratio = 0.95});
}

[[nodiscard]] const mirakana::MemoryClassDiagnosticsSummary*
find_memory_class_summary(const mirakana::MemoryDiagnosticsSummary& summary,
                          mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    for (const auto& class_summary : summary.class_summaries) {
        if (class_summary.lifetime_class == lifetime_class) {
            return &class_summary;
        }
    }
    return nullptr;
}

[[nodiscard]] std::uint64_t memory_class_bytes(const mirakana::MemoryDiagnosticsSummary& summary,
                                               mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    const auto* class_summary = find_memory_class_summary(summary, lifetime_class);
    return class_summary == nullptr ? 0U : class_summary->bytes;
}

[[nodiscard]] std::uint64_t memory_class_allocations(const mirakana::MemoryDiagnosticsSummary& summary,
                                                     mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    const auto* class_summary = find_memory_class_summary(summary, lifetime_class);
    return class_summary == nullptr ? 0U : class_summary->allocation_count;
}

[[nodiscard]] std::uint64_t memory_class_budget(const mirakana::MemoryDiagnosticsSummary& summary,
                                                mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    const auto* class_summary = find_memory_class_summary(summary, lifetime_class);
    return class_summary == nullptr ? 0U : class_summary->budget_bytes;
}

[[nodiscard]] std::string_view memory_class_pressure(const mirakana::MemoryDiagnosticsSummary& summary,
                                                     mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    const auto* class_summary = find_memory_class_summary(summary, lifetime_class);
    return class_summary == nullptr ? "missing" : mirakana::memory_budget_pressure_label(class_summary->pressure);
}

[[nodiscard]] bool memory_diagnostic_code_present(const mirakana::MemoryDiagnosticsSummary& summary,
                                                  mirakana::MemoryDiagnosticsCode code) noexcept {
    for (const auto diagnostic_code : summary.diagnostic_codes) {
        if (diagnostic_code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::uint64_t
memory_diagnostics_budgeted_class_count(const mirakana::MemoryDiagnosticsSummary& summary) noexcept {
    std::uint64_t count = 0U;
    for (const auto& class_summary : summary.class_summaries) {
        if (class_summary.budget_bytes > 0U) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::uint64_t memory_diagnostics_pressure_class_count(const mirakana::MemoryDiagnosticsSummary& summary,
                                                                    mirakana::MemoryBudgetPressure pressure) noexcept {
    std::uint64_t count = 0U;
    for (const auto& class_summary : summary.class_summaries) {
        if (class_summary.pressure == pressure) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::size_t memory_diagnostics_transient_gpu_aliasing_barriers(
    bool requested, const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    return requested ? evidence.aliasing_barriers_recorded : 0U;
}

[[nodiscard]] bool memory_diagnostics_transient_gpu_framegraph_aliasing_ready(
    bool requested, const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    return requested && evidence.ready && evidence.aliasing_barriers_recorded > 0U;
}

[[nodiscard]] mirakana::Win32DesktopPresentationQualityGateDesc
make_renderer_quality_gate_desc(const DesktopRuntimeGameOptions& options) noexcept {
    mirakana::Win32DesktopPresentationQualityGateDesc desc;
    if (options.require_renderer_quality_gates) {
        desc.require_scene_gpu_bindings = true;
        desc.require_postprocess = true;
        desc.require_postprocess_depth_input = true;
        desc.require_directional_shadow = true;
        desc.require_directional_shadow_filtering = true;
        desc.expected_frames = options.max_frames;
    }
    return desc;
}

[[nodiscard]] std::string_view lighting_shadow_policy_status_name(bool requested, bool ready) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return ready ? "ready" : "blocked";
}

[[nodiscard]] std::string_view
frame_graph_multi_queue_status_name(bool requested,
                                    const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return evidence.ready ? "ready" : "blocked";
}

[[nodiscard]] mirakana::JobSchedulingExecutionEvidence build_package_job_scheduling_evidence(bool requested) {
    if (!requested) {
        return {};
    }

    const std::array<mirakana::JobWorkerTopologyRow, 1> topologies{
        mirakana::JobWorkerTopologyRow{.name = "desktop_runtime.package_pool",
                                       .logical_processor_count = 8,
                                       .worker_count = 2,
                                       .queue_count = 2,
                                       .processor_group_count = 1,
                                       .numa_node_count = 1,
                                       .processor_groups_accounted_for = true,
                                       .numa_topology_known = true},
    };
    const std::array<mirakana::JobSchedulingWorkItemRow, 3> work_items{
        mirakana::JobSchedulingWorkItemRow{.job_id = "package.load_scene",
                                           .worker_id = 0,
                                           .batch_size = 32,
                                           .scratch_bytes = 1024,
                                           .worker_local_output_count = 1,
                                           .merge_order = 0},
        mirakana::JobSchedulingWorkItemRow{.job_id = "package.resolve_gameplay",
                                           .worker_id = 1,
                                           .dependency_job_ids = {"package.load_scene"},
                                           .batch_size = 16,
                                           .scratch_bytes = 512,
                                           .worker_local_output_count = 1,
                                           .merge_order = 1},
        mirakana::JobSchedulingWorkItemRow{.job_id = "package.submit_render_intent",
                                           .worker_id = 0,
                                           .dependency_job_ids = {"package.resolve_gameplay"},
                                           .batch_size = 8,
                                           .scratch_bytes = 256,
                                           .worker_local_output_count = 1,
                                           .merge_order = 2},
    };
    return mirakana::build_job_scheduling_execution_evidence(
        topologies, work_items,
        mirakana::JobSchedulingExecutionOptions{.queue_capacity_per_worker = 4,
                                                .minimum_batch_size = 4,
                                                .maximum_batch_size = 64,
                                                .scratch_budget_bytes_per_worker = 4096,
                                                .frame_index = 0});
}

[[nodiscard]] bool job_scheduling_evidence_ready(const mirakana::JobSchedulingExecutionEvidence& evidence) noexcept {
    return evidence.scheduling_summary.status == mirakana::JobSchedulingDiagnosticsStatus::ready &&
           evidence.scratch_summary.status == mirakana::MemoryDiagnosticsStatus::ready &&
           evidence.queue_rows.size() == 2U && evidence.worker_scratch_rows.size() == 2U &&
           evidence.execution_order.size() == 3U && evidence.scheduling_summary.total_submitted_jobs == 3U &&
           evidence.scheduling_summary.total_completed_jobs == 3U &&
           evidence.scheduling_summary.total_deterministic_merge_count == 3U &&
           evidence.scheduling_summary.total_nondeterministic_merge_count == 0U &&
           evidence.scheduling_summary.total_scratch_misuse_count == 0U;
}

[[nodiscard]] std::string_view
job_scheduling_evidence_status_name(bool requested, const mirakana::JobSchedulingExecutionEvidence& evidence) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return job_scheduling_evidence_ready(evidence) ? "ready" : "blocked";
}

[[nodiscard]] mirakana::JobExecutionTopologyPolicyDesc make_package_job_execution_topology_policy_desc() {
    return mirakana::JobExecutionTopologyPolicyDesc{.name = "desktop_runtime.package_execution_pool",
                                                    .observed_logical_processor_count = 8,
                                                    .fallback_logical_processor_count = 1,
                                                    .worker_count_limit = 2,
                                                    .reserved_logical_processor_count = 1,
                                                    .queue_capacity_per_worker = 4,
                                                    .scratch_budget_bytes_per_worker = 4096,
                                                    .frame_index = 0,
                                                    .processor_group_count = 1,
                                                    .numa_node_count = 1,
                                                    .processor_groups_accounted_for = true,
                                                    .numa_topology_known = true};
}

[[nodiscard]] mirakana::JobExecutionTopologyPolicy
build_package_job_execution_topology_policy_evidence(bool requested) {
    if (!requested) {
        return {};
    }
    return mirakana::select_job_execution_topology_policy(make_package_job_execution_topology_policy_desc());
}

[[nodiscard]] bool job_execution_topology_policy_ready(const mirakana::JobExecutionTopologyPolicy& policy) noexcept {
    return policy.status == mirakana::JobExecutionTopologyPolicyStatus::ready &&
           policy.observed_logical_processor_count == 8U && policy.effective_logical_processor_count == 8U &&
           policy.selected_worker_count == 2U && policy.worker_count_limit == 2U &&
           policy.reserved_logical_processor_count == 1U && policy.worker_count_limited_by_cap &&
           !policy.hardware_concurrency_fallback_used && !policy.requested_worker_count_used &&
           !policy.worker_count_clamped_to_logical_processors && !policy.processor_group_policy_applied &&
           !policy.numa_policy_applied && !policy.affinity_policy_applied && !policy.simd_dispatch_applied &&
           !policy.gpu_async_overlap_applied && policy.topology_row.processor_group_count == 1U &&
           policy.topology_row.numa_node_count == 1U && policy.topology_row.processor_groups_accounted_for &&
           policy.topology_row.numa_topology_known && policy.diagnostics.empty();
}

[[nodiscard]] std::string_view
job_execution_topology_policy_status_name(bool requested, const mirakana::JobExecutionTopologyPolicy& policy) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return job_execution_topology_policy_ready(policy) ? "ready" : "blocked";
}

struct JobExecutionFoundationEvidence {
    bool requested{false};
    mirakana::JobExecutionPoolStatus pool_status{mirakana::JobExecutionPoolStatus::invalid_configuration};
    mirakana::JobExecutionRunResult run_result;
    std::uint64_t task_side_effects{0};
};

[[nodiscard]] JobExecutionFoundationEvidence build_package_job_execution_foundation_evidence(bool requested) {
    JobExecutionFoundationEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    std::atomic_uint64_t task_side_effects{0};
    auto policy_desc = make_package_job_execution_topology_policy_desc();
    policy_desc.enable_work_stealing = true;
    const auto topology_policy = mirakana::select_job_execution_topology_policy(policy_desc);
    if (!topology_policy.ready()) {
        return evidence;
    }
    auto pool = mirakana::JobExecutionPool(topology_policy.pool_desc);
    evidence.pool_status = pool.status();
    if (evidence.pool_status != mirakana::JobExecutionPoolStatus::ready) {
        return evidence;
    }

    const auto body = [&task_side_effects](mirakana::JobExecutionContext& context) {
        if (context.stop_token.stop_requested()) {
            throw std::runtime_error("job execution foundation task received an unexpected stop request");
        }
        const auto lease = context.scratch.acquire(32, 16, context.worker_id);
        if (!lease.valid()) {
            throw std::runtime_error("job execution foundation worker scratch lease failed");
        }
        lease.bytes.front() = std::byte{0x2A};
        const auto release_status = context.scratch.release(lease, context.worker_id);
        if (release_status != mirakana::ScratchLeaseStatus::released) {
            throw std::runtime_error("job execution foundation worker scratch release failed");
        }
        task_side_effects.fetch_add(1, std::memory_order_relaxed);
    };

    auto batch = mirakana::JobExecutionBatchDesc{};
    batch.tasks = {
        mirakana::JobExecutionTaskDesc{.evidence =
                                           mirakana::JobSchedulingWorkItemRow{.job_id = "package.execute_load_scene",
                                                                              .worker_id = 0,
                                                                              .batch_size = 32,
                                                                              .scratch_bytes = 32,
                                                                              .worker_local_output_count = 1,
                                                                              .merge_order = 0},
                                       .body = body},
        mirakana::JobExecutionTaskDesc{
            .evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.execute_resolve_gameplay",
                                                           .worker_id = 1,
                                                           .dependency_job_ids = {"package.execute_load_scene"},
                                                           .batch_size = 16,
                                                           .scratch_bytes = 32,
                                                           .worker_local_output_count = 1,
                                                           .merge_order = 1},
            .body = body},
        mirakana::JobExecutionTaskDesc{
            .evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.execute_submit_render_intent",
                                                           .worker_id = 0,
                                                           .dependency_job_ids = {"package.execute_resolve_gameplay"},
                                                           .batch_size = 8,
                                                           .scratch_bytes = 32,
                                                           .worker_local_output_count = 1,
                                                           .merge_order = 2},
            .body = body},
    };
    batch.options.minimum_batch_size = 4;
    batch.options.maximum_batch_size = 64;

    evidence.run_result = pool.execute(batch);
    evidence.task_side_effects = task_side_effects.load(std::memory_order_relaxed);
    return evidence;
}

[[nodiscard]] std::size_t
job_execution_foundation_diagnostic_count(const JobExecutionFoundationEvidence& evidence) noexcept {
    return evidence.run_result.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scheduling_summary.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scratch_summary.diagnostics.size();
}

[[nodiscard]] bool job_execution_foundation_ready(const JobExecutionFoundationEvidence& evidence) noexcept {
    const auto& run = evidence.run_result;
    const auto& scheduling = run.scheduling_evidence.scheduling_summary;
    const auto& scratch = run.scheduling_evidence.scratch_summary;
    return evidence.requested && evidence.pool_status == mirakana::JobExecutionPoolStatus::ready &&
           run.status == mirakana::JobExecutionRunStatus::ready && run.worker_threads_started == 2U &&
           scheduling.total_submitted_jobs == 3U && run.tasks_executed == 3U && run.tasks_failed == 0U &&
           evidence.task_side_effects == 3U && run.scheduling_evidence.execution_order.size() == 3U &&
           run.scheduling_evidence.queue_rows.size() == 2U && scheduling.total_deterministic_merge_count == 3U &&
           scheduling.total_nondeterministic_merge_count == 0U && scheduling.total_blocked_dependency_count == 0U &&
           scheduling.total_dependency_cycle_count == 0U && scheduling.total_queue_overflow_count == 0U &&
           scratch.status == mirakana::MemoryDiagnosticsStatus::ready &&
           run.scheduling_evidence.worker_scratch_rows.size() == 2U && scratch.total_bytes > 0U &&
           scratch.high_water_bytes > 0U && job_execution_foundation_diagnostic_count(evidence) == 0U;
}

[[nodiscard]] std::string_view
job_execution_foundation_status_name(const JobExecutionFoundationEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return job_execution_foundation_ready(evidence) ? "ready" : "blocked";
}

struct JobExecutionWorkStealingEvidence {
    bool requested{false};
    mirakana::JobExecutionPoolStatus pool_status{mirakana::JobExecutionPoolStatus::invalid_configuration};
    mirakana::JobExecutionRunResult run_result;
    std::uint64_t task_side_effects{0};
};

[[nodiscard]] JobExecutionWorkStealingEvidence build_package_job_execution_work_stealing_evidence(bool requested) {
    JobExecutionWorkStealingEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    auto policy_desc = make_package_job_execution_topology_policy_desc();
    policy_desc.enable_work_stealing = true;
    const auto topology_policy = mirakana::select_job_execution_topology_policy(policy_desc);
    if (!topology_policy.ready()) {
        return evidence;
    }
    auto pool = mirakana::JobExecutionPool(topology_policy.pool_desc);
    evidence.pool_status = pool.status();
    if (evidence.pool_status != mirakana::JobExecutionPoolStatus::ready) {
        return evidence;
    }

    std::mutex observed_mutex;
    std::condition_variable observed_cv;
    std::uint32_t started_task_count{0};
    std::atomic_uint64_t task_side_effects{0};
    const auto body = [&](mirakana::JobExecutionContext& context) {
        const auto lease = context.scratch.acquire(32, 16, context.worker_id);
        if (!lease.valid()) {
            throw std::runtime_error("job execution work stealing worker scratch lease failed");
        }
        lease.bytes.front() = std::byte{0x42};
        const auto release_status = context.scratch.release(lease, context.worker_id);
        if (release_status != mirakana::ScratchLeaseStatus::released) {
            throw std::runtime_error("job execution work stealing worker scratch release failed");
        }

        bool both_tasks_started = false;
        {
            std::unique_lock lock(observed_mutex);
            ++started_task_count;
            observed_cv.notify_all();
            both_tasks_started = observed_cv.wait_for(lock, std::chrono::seconds{2},
                                                      [&started_task_count] { return started_task_count >= 2U; });
        }
        if (!both_tasks_started) {
            throw std::runtime_error("job execution work stealing task did not run concurrently");
        }
        task_side_effects.fetch_add(1, std::memory_order_relaxed);
    };

    auto batch = mirakana::JobExecutionBatchDesc{};
    batch.tasks = {
        mirakana::JobExecutionTaskDesc{.evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.steal_prepare",
                                                                                      .worker_id = 0,
                                                                                      .batch_size = 32,
                                                                                      .scratch_bytes = 32,
                                                                                      .worker_local_output_count = 1,
                                                                                      .merge_order = 0},
                                       .body = body},
        mirakana::JobExecutionTaskDesc{.evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.steal_execute",
                                                                                      .worker_id = 0,
                                                                                      .batch_size = 32,
                                                                                      .scratch_bytes = 32,
                                                                                      .worker_local_output_count = 1,
                                                                                      .merge_order = 1},
                                       .body = body},
    };
    batch.options.minimum_batch_size = 4;
    batch.options.maximum_batch_size = 64;

    evidence.run_result = pool.execute(batch);
    evidence.task_side_effects = task_side_effects.load(std::memory_order_relaxed);
    return evidence;
}

[[nodiscard]] std::size_t
job_execution_work_stealing_diagnostic_count(const JobExecutionWorkStealingEvidence& evidence) noexcept {
    return evidence.run_result.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scheduling_summary.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scratch_summary.diagnostics.size();
}

[[nodiscard]] bool job_execution_work_stealing_ready(const JobExecutionWorkStealingEvidence& evidence) noexcept {
    const auto& run = evidence.run_result;
    const auto& scheduling = run.scheduling_evidence.scheduling_summary;
    const auto& scratch = run.scheduling_evidence.scratch_summary;
    return evidence.requested && evidence.pool_status == mirakana::JobExecutionPoolStatus::ready &&
           run.status == mirakana::JobExecutionRunStatus::ready && run.worker_threads_started == 2U &&
           scheduling.total_submitted_jobs == 2U && run.tasks_executed == 2U && run.tasks_failed == 0U &&
           evidence.task_side_effects == 2U && run.scheduling_evidence.execution_order.size() == 2U &&
           run.scheduling_evidence.queue_rows.size() == 2U && run.work_stealing_applied &&
           run.steal_attempt_count >= run.steal_success_count && run.steal_success_count > 0U &&
           scheduling.total_steal_attempt_count >= scheduling.total_steal_success_count &&
           scheduling.total_steal_success_count > 0U && scheduling.total_deterministic_merge_count == 2U &&
           scheduling.total_nondeterministic_merge_count == 0U && scheduling.total_blocked_dependency_count == 0U &&
           scheduling.total_dependency_cycle_count == 0U && scheduling.total_queue_overflow_count == 0U &&
           scratch.status == mirakana::MemoryDiagnosticsStatus::ready &&
           run.scheduling_evidence.worker_scratch_rows.size() == 2U && scratch.total_bytes > 0U &&
           scratch.high_water_bytes > 0U && job_execution_work_stealing_diagnostic_count(evidence) == 0U;
}

[[nodiscard]] std::string_view
job_execution_work_stealing_status_name(const JobExecutionWorkStealingEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return job_execution_work_stealing_ready(evidence) ? "ready" : "blocked";
}

struct JobExecutionPlacementPolicyEvidence {
    bool requested{false};
    mirakana::JobExecutionPlacementPolicy ready_policy;
    mirakana::JobExecutionPlacementPolicy host_evidence_policy;
};

[[nodiscard]] JobExecutionPlacementPolicyEvidence
build_package_job_execution_placement_policy_evidence(bool requested) {
    JobExecutionPlacementPolicyEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    auto topology_desc = make_package_job_execution_topology_policy_desc();
    topology_desc.enable_work_stealing = true;
    const auto topology_policy = mirakana::select_job_execution_topology_policy(topology_desc);
    if (!topology_policy.ready()) {
        return evidence;
    }

    evidence.ready_policy = mirakana::select_job_execution_placement_policy(mirakana::JobExecutionPlacementPolicyDesc{
        .name = "desktop_runtime.package_execution_placement",
        .topology_policy = topology_policy,
        .requested_mode = mirakana::JobExecutionPlacementPolicyMode::os_default,
    });
    evidence.host_evidence_policy =
        mirakana::select_job_execution_placement_policy(mirakana::JobExecutionPlacementPolicyDesc{
            .name = "desktop_runtime.package_execution_placement.numa_probe",
            .topology_policy = topology_policy,
            .requested_mode = mirakana::JobExecutionPlacementPolicyMode::prefer_local_numa,
        });
    return evidence;
}

[[nodiscard]] bool job_execution_placement_policy_ready(const JobExecutionPlacementPolicyEvidence& evidence) noexcept {
    const auto& policy = evidence.ready_policy;
    const auto& host_evidence_policy = evidence.host_evidence_policy;
    return evidence.requested && policy.status == mirakana::JobExecutionPlacementPolicyStatus::ready &&
           policy.requested_mode == mirakana::JobExecutionPlacementPolicyMode::os_default &&
           policy.selected_mode == mirakana::JobExecutionPlacementPolicyMode::os_default &&
           policy.inherited_worker_count == 2U && policy.pool_desc.work_stealing_enabled &&
           policy.numa_node_count == 1U && policy.performance_core_count == 0U && policy.efficiency_core_count == 0U &&
           !policy.smt_sibling_topology_known && !policy.affinity_policy_applied && !policy.numa_policy_applied &&
           !policy.simd_dispatch_applied && !policy.gpu_async_overlap_applied && policy.diagnostics.empty() &&
           host_evidence_policy.status == mirakana::JobExecutionPlacementPolicyStatus::host_evidence_required &&
           host_evidence_policy.diagnostic_codes.size() == 1U &&
           std::ranges::find(host_evidence_policy.diagnostic_codes,
                             mirakana::JobExecutionPlacementPolicyDiagnosticCode::missing_numa_evidence) !=
               host_evidence_policy.diagnostic_codes.end();
}

[[nodiscard]] std::string_view
job_execution_placement_policy_status_name(const JobExecutionPlacementPolicyEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return job_execution_placement_policy_ready(evidence) ? "ready" : "blocked";
}

struct WindowsCpuSetWorkerPlacementEvidence {
    bool requested{false};
    mirakana::JobExecutionPlacementPolicyMode selected_mode{mirakana::JobExecutionPlacementPolicyMode::os_default};
    mirakana::win32::Win32CpuSetWorkerPlacementPlan placement_plan;
    mirakana::JobExecutionPoolStatus pool_status{mirakana::JobExecutionPoolStatus::invalid_configuration};
    mirakana::JobExecutionRunResult run_result;
    std::uint64_t task_side_effects{0};
};

[[nodiscard]] WindowsCpuSetWorkerPlacementEvidence
build_package_windows_cpu_set_worker_placement_evidence(bool requested, mirakana::JobExecutionPlacementPolicyMode mode,
                                                        std::string_view job_prefix) {
    WindowsCpuSetWorkerPlacementEvidence evidence;
    evidence.requested = requested;
    evidence.selected_mode = mode;
    if (!requested) {
        return evidence;
    }

    const auto topology_policy =
        mirakana::select_job_execution_topology_policy(make_package_job_execution_topology_policy_desc());
    if (!topology_policy.ready()) {
        return evidence;
    }

    auto cpu_sets = mirakana::win32::query_win32_cpu_sets();
    evidence.placement_plan =
        mirakana::win32::select_win32_cpu_set_worker_placement(mirakana::win32::Win32CpuSetWorkerPlacementDesc{
            .cpu_sets = std::move(cpu_sets),
            .worker_count = topology_policy.selected_worker_count,
            .mode = mode,
        });
    if (!evidence.placement_plan.ready()) {
        return evidence;
    }

    auto pool_desc = topology_policy.pool_desc;
    pool_desc.placement_requested_mode = mode;
    pool_desc.placement_selected_mode = mode;
    pool_desc.worker_placement_callback =
        mirakana::win32::make_win32_cpu_set_worker_placement_callback(evidence.placement_plan);

    auto pool = mirakana::JobExecutionPool(pool_desc);
    evidence.pool_status = pool.status();
    if (evidence.pool_status != mirakana::JobExecutionPoolStatus::ready) {
        return evidence;
    }

    std::atomic_uint64_t task_side_effects{0};
    const auto body = [&task_side_effects](mirakana::JobExecutionContext& context) {
        if (context.stop_token.stop_requested()) {
            throw std::runtime_error("windows CPU set worker placement task received an unexpected stop request");
        }
        const auto lease = context.scratch.acquire(32, 16, context.worker_id);
        if (!lease.valid()) {
            throw std::runtime_error("windows CPU set worker placement scratch lease failed");
        }
        lease.bytes.front() = std::byte{0x7B};
        const auto release_status = context.scratch.release(lease, context.worker_id);
        if (release_status != mirakana::ScratchLeaseStatus::released) {
            throw std::runtime_error("windows CPU set worker placement scratch release failed");
        }
        task_side_effects.fetch_add(1, std::memory_order_relaxed);
    };

    const auto prepare_job_id = std::string{job_prefix} + ".prepare";
    const auto execute_job_id = std::string{job_prefix} + ".execute";
    auto batch = mirakana::JobExecutionBatchDesc{};
    batch.tasks = {
        mirakana::JobExecutionTaskDesc{.evidence = mirakana::JobSchedulingWorkItemRow{.job_id = prepare_job_id,
                                                                                      .worker_id = 0,
                                                                                      .batch_size = 32,
                                                                                      .scratch_bytes = 32,
                                                                                      .worker_local_output_count = 1,
                                                                                      .merge_order = 0},
                                       .body = body},
        mirakana::JobExecutionTaskDesc{.evidence = mirakana::JobSchedulingWorkItemRow{.job_id = execute_job_id,
                                                                                      .worker_id = 1,
                                                                                      .batch_size = 32,
                                                                                      .scratch_bytes = 32,
                                                                                      .worker_local_output_count = 1,
                                                                                      .merge_order = 1},
                                       .body = body},
    };
    batch.options.minimum_batch_size = 4;
    batch.options.maximum_batch_size = 64;

    evidence.run_result = pool.execute(batch);
    evidence.task_side_effects = task_side_effects.load(std::memory_order_relaxed);
    return evidence;
}

[[nodiscard]] std::size_t
windows_cpu_set_worker_placement_diagnostic_count(const WindowsCpuSetWorkerPlacementEvidence& evidence) noexcept {
    return evidence.placement_plan.diagnostics.size() + evidence.run_result.diagnostics.size() +
           evidence.run_result.worker_placement_diagnostic_count +
           evidence.run_result.scheduling_evidence.scheduling_summary.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scratch_summary.diagnostics.size();
}

[[nodiscard]] bool
windows_cpu_set_worker_placement_ready(const WindowsCpuSetWorkerPlacementEvidence& evidence) noexcept {
    const auto& run = evidence.run_result;
    const auto& scheduling = run.scheduling_evidence.scheduling_summary;
    const auto& scratch = run.scheduling_evidence.scratch_summary;
    return evidence.requested && evidence.placement_plan.ready() &&
           evidence.placement_plan.selected_cpu_set_count > 0U && evidence.placement_plan.worker_rows.size() == 2U &&
           evidence.pool_status == mirakana::JobExecutionPoolStatus::ready &&
           run.status == mirakana::JobExecutionRunStatus::ready && run.worker_threads_started == 2U &&
           run.worker_placement_attempt_count == 2U && run.worker_placement_applied_count == 2U &&
           run.worker_placement_diagnostic_count == 0U && run.worker_placement_selected_cpu_set_count == 2U &&
           scheduling.worker_topology_row_count == 1U && scheduling.total_submitted_jobs == 2U &&
           run.tasks_executed == 2U && run.tasks_failed == 0U && evidence.task_side_effects == 2U &&
           run.scheduling_evidence.execution_order.size() == 2U && run.scheduling_evidence.queue_rows.size() == 2U &&
           scratch.status == mirakana::MemoryDiagnosticsStatus::ready &&
           run.scheduling_evidence.worker_scratch_rows.size() == 2U &&
           windows_cpu_set_worker_placement_diagnostic_count(evidence) == 0U;
}

[[nodiscard]] std::string_view
windows_cpu_set_worker_placement_status_name(const WindowsCpuSetWorkerPlacementEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return windows_cpu_set_worker_placement_ready(evidence) ? "ready" : "blocked";
}

[[nodiscard]] bool
windows_cpu_set_smt_worker_placement_ready(const WindowsCpuSetWorkerPlacementEvidence& evidence) noexcept {
    return windows_cpu_set_worker_placement_ready(evidence) &&
           evidence.selected_mode == mirakana::JobExecutionPlacementPolicyMode::avoid_smt_siblings &&
           evidence.placement_plan.distinct_core_count > 0U && evidence.placement_plan.smt_sibling_topology_known &&
           evidence.placement_plan.smt_sibling_cpu_set_count > 0U && evidence.placement_plan.smt_sibling_policy_applied;
}

[[nodiscard]] std::string_view
windows_cpu_set_smt_worker_placement_status_name(const WindowsCpuSetWorkerPlacementEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return windows_cpu_set_smt_worker_placement_ready(evidence) ? "ready" : "blocked";
}

struct SimdDispatchPolicyEvidence {
    bool requested{false};
    mirakana::SimdDotProductEvidence dot_product;
};

[[nodiscard]] SimdDispatchPolicyEvidence build_package_simd_dispatch_policy_evidence(bool requested) {
    SimdDispatchPolicyEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    constexpr std::array lhs{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F};
    constexpr std::array rhs{8.0F, 7.0F, 6.0F, 5.0F, 4.0F, 3.0F, 2.0F, 1.0F};
    evidence.dot_product = mirakana::build_simd_dot_product_evidence(
        lhs, rhs, mirakana::SimdDispatchPolicyDesc{.requested_lane = mirakana::CpuSimdLaneRequest::auto_select});
    return evidence;
}

[[nodiscard]] bool simd_dispatch_policy_ready(const SimdDispatchPolicyEvidence& evidence) noexcept {
    const auto& dot = evidence.dot_product;
    return evidence.requested && dot.policy.ready() && dot.input_count == 8U && dot.result == 120.0F &&
           dot.span_inputs_used && !dot.raw_pointers_retained && dot.diagnostics.empty() &&
           !dot.policy.gpu_async_overlap_applied && !dot.policy.cuda_path_used && !dot.policy.hip_path_used &&
           !dot.policy.sycl_path_used;
}

[[nodiscard]] std::string_view simd_dispatch_policy_status_name(const SimdDispatchPolicyEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return simd_dispatch_policy_ready(evidence) ? "ready" : "blocked";
}

[[nodiscard]] mirakana::FrameGraphRhiMultiQueuePackageEvidence
run_frame_graph_multi_queue_package_evidence(mirakana::Win32DesktopPresentation& presentation) {
    auto* device = presentation.scene_pbr_frame_rhi_device();
    if (device != nullptr) {
        return mirakana::execute_frame_graph_rhi_multi_queue_package_evidence(*device);
    }

    mirakana::FrameGraphRhiMultiQueuePackageEvidence evidence;
    evidence.diagnostics.push_back(mirakana::FrameGraphDiagnostic{
        .code = mirakana::FrameGraphDiagnosticCode::invalid_pass,
        .pass = "framegraph_multiqueue",
        .resource = "package.alias.early",
        .message = "Frame Graph multi-queue package evidence requires a native RHI device",
    });
    return evidence;
}

void print_package_failures(const std::vector<mirakana::runtime::RuntimeAssetPackageLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime package failure asset=" << failure.asset.value << " path=" << failure.path << ": "
                  << failure.diagnostic << '\n';
    }
}

void print_scene_failures(const std::vector<mirakana::RuntimeSceneRenderLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime scene failure asset=" << failure.asset.value << ": " << failure.diagnostic << '\n';
    }
}

[[nodiscard]] std::filesystem::path executable_directory(const char* executable_path) {
    try {
        if (executable_path != nullptr && !std::string_view{executable_path}.empty()) {
            const auto absolute_path = std::filesystem::absolute(std::filesystem::path{executable_path});
            if (absolute_path.has_parent_path()) {
                return absolute_path.parent_path();
            }
        }
        return std::filesystem::current_path();
    } catch (const std::exception&) {
        return std::filesystem::path{"."};
    }
}

[[nodiscard]] bool verify_required_config(const char* executable_path, std::string_view config_path) {
    if (config_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(config_path)) {
            std::cerr << "required config was not found: " << config_path << '\n';
            return false;
        }

        const auto config_text = filesystem.read_text(config_path);
        if (config_text.empty()) {
            std::cerr << "required config is empty: " << config_path << '\n';
            return false;
        }
        if (!config_text.starts_with(kExpectedConfigFormat)) {
            std::cerr << "required config has unexpected format: " << config_path << '\n';
            return false;
        }
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required config '" << config_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] bool
load_required_scene_package(const char* executable_path, std::string_view package_path,
                            std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                            std::optional<mirakana::RuntimeSceneRenderInstance>& scene,
                            std::vector<mirakana::AnimationJointTrack3dDesc>& quaternion_animation_tracks) {
    if (package_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(package_path)) {
            std::cerr << "required scene package was not found: " << package_path << '\n';
            return false;
        }

        auto package_result =
            mirakana::runtime::load_runtime_asset_package(filesystem, mirakana::runtime::RuntimeAssetPackageDesc{
                                                                          .index_path = std::string{package_path},
                                                                          .content_root = {},
                                                                      });
        if (!package_result.succeeded()) {
            print_package_failures(package_result.failures);
            return false;
        }

        auto instance =
            mirakana::instantiate_runtime_scene_render_data(package_result.package, packaged_scene_asset_id());
        if (!instance.succeeded()) {
            print_scene_failures(instance.failures);
            return false;
        }
        if (instance.render_packet.meshes.empty()) {
            std::cerr << "runtime scene package did not produce renderable meshes: " << package_path << '\n';
            return false;
        }
        if (instance.material_palette.count() == 0) {
            std::cerr << "runtime scene package did not resolve scene materials: " << package_path << '\n';
            return false;
        }

        const auto* quaternion_animation_record = package_result.package.find(packaged_quaternion_animation_asset_id());
        if (quaternion_animation_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged quaternion animation clip: " << package_path
                      << '\n';
            return false;
        }
        const auto quaternion_payload =
            mirakana::runtime::runtime_animation_quaternion_clip_payload(*quaternion_animation_record);
        if (!quaternion_payload.succeeded()) {
            std::cerr << "runtime scene package quaternion animation clip is invalid: " << quaternion_payload.diagnostic
                      << '\n';
            return false;
        }
        auto decoded_tracks = make_quaternion_animation_tracks(quaternion_payload.payload);
        if (!mirakana::is_valid_animation_joint_tracks_3d(packaged_quaternion_animation_skeleton(), decoded_tracks)) {
            std::cerr << "runtime scene package quaternion animation clip does not match its smoke skeleton: "
                      << package_path << '\n';
            return false;
        }

        quaternion_animation_tracks = std::move(decoded_tracks);
        runtime_package = std::move(package_result.package);
        scene = std::move(instance);
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required scene package '" << package_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_postprocess_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimePostprocessVertexShaderPath},
        .fragment_path = std::string{kRuntimePostprocessFragmentShaderPath},
        .vertex_entry_point = "vs_postprocess",
        .fragment_entry_point = "ps_postprocess",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_environment_ibl_sample_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeEnvironmentIblSampleVertexShaderPath},
        .fragment_path = std::string{kRuntimeEnvironmentIblSampleFragmentShaderPath},
        .vertex_entry_point = "vs_environment_ibl_sample",
        .fragment_entry_point = "ps_environment_ibl_sample",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_environment_ibl_sample_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeEnvironmentIblSampleVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeEnvironmentIblSampleVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_environment_ibl_sample",
        .fragment_entry_point = "ps_environment_ibl_sample",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_cloud_layer_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeCloudLayerVertexShaderPath},
        .fragment_path = std::string{kRuntimeCloudLayerFragmentShaderPath},
        .vertex_entry_point = "main_vs",
        .fragment_entry_point = "main_ps",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_precipitation_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimePrecipitationVertexShaderPath},
        .fragment_path = std::string{kRuntimePrecipitationFragmentShaderPath},
        .vertex_entry_point = "main_vs",
        .fragment_entry_point = "main_ps",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_precipitation_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimePrecipitationVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimePrecipitationVulkanFragmentShaderPath},
        .vertex_entry_point = "main_vs",
        .fragment_entry_point = "main_ps",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_volumetric_cloud_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeEnvironmentVolumetricCloudVertexShaderPath},
        .fragment_path = std::string{kRuntimeEnvironmentVolumetricCloudFragmentShaderPath},
        .vertex_entry_point = "main_vs",
        .fragment_entry_point = "main_ps",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_volumetric_cloud_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeEnvironmentVolumetricCloudVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeEnvironmentVolumetricCloudVulkanFragmentShaderPath},
        .vertex_entry_point = "main_vs",
        .fragment_entry_point = "main_ps",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_postprocess_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimePostprocessVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimePostprocessVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_postprocess",
        .fragment_entry_point = "ps_postprocess",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_environment_fog_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeEnvironmentFogVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeEnvironmentFogVulkanFragmentShaderPath},
        .vertex_entry_point = "height_fog_vs_main",
        .fragment_entry_point = "height_fog_ps_main",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowReceiverFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowReceiverVulkanFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_shadow_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeShadowVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowFragmentShaderPath},
        .vertex_entry_point = "vs_shadow",
        .fragment_entry_point = "ps_shadow",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_shadow_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeShadowVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_shadow",
        .fragment_entry_point = "ps_shadow",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_native_ui_overlay_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeNativeUiOverlayVertexShaderPath},
        .fragment_path = std::string{kRuntimeNativeUiOverlayFragmentShaderPath},
        .vertex_entry_point = "vs_native_ui_overlay",
        .fragment_entry_point = "ps_native_ui_overlay",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_native_ui_overlay_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeNativeUiOverlayVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeNativeUiOverlayVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_native_ui_overlay",
        .fragment_entry_point = "ps_native_ui_overlay",
    });
}

[[nodiscard]] mirakana::Win32DesktopPresentationShaderBytecode
to_presentation_shader_bytecode(const mirakana::DesktopShaderBytecodeBlob& bytecode) noexcept {
    return mirakana::Win32DesktopPresentationShaderBytecode{
        .entry_point = bytecode.entry_point,
        .bytecode = std::span<const std::uint8_t>{bytecode.bytecode.data(), bytecode.bytecode.size()},
    };
}

[[nodiscard]] mirakana::DesktopShaderBytecodeBlob load_packaged_single_shader_blob(const char* executable_path,
                                                                                   std::string_view relative_path,
                                                                                   std::string_view entry_point) {
    mirakana::DesktopShaderBytecodeBlob blob;
    blob.entry_point = std::string{entry_point};
    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        const std::string path{relative_path};
        if (!filesystem.exists(path)) {
            return blob;
        }
        const auto text = filesystem.read_text(path);
        blob.bytecode.assign(text.begin(), text.end());
    } catch (const std::exception&) {
        blob.bytecode.clear();
    }
    return blob;
}

[[nodiscard]] std::string_view status_name(mirakana::DesktopRunStatus status) noexcept {
    switch (status) {
    case mirakana::DesktopRunStatus::completed:
        return "completed";
    case mirakana::DesktopRunStatus::stopped_by_app:
        return "stopped_by_app";
    case mirakana::DesktopRunStatus::window_closed:
        return "window_closed";
    case mirakana::DesktopRunStatus::lifecycle_quit:
        return "lifecycle_quit";
    }
    return "unknown";
}

void print_presentation_report(std::string_view prefix, const mirakana::Win32DesktopGameHost& host) {
    const auto report = host.presentation_report();
    const auto postprocess_policy = mirakana::evaluate_win32_desktop_presentation_postprocess_policy(report);
    std::cout << prefix << " presentation_report=requested="
              << mirakana::win32_desktop_presentation_backend_name(report.requested_backend)
              << " selected=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
              << " fallback=" << mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " diagnostics=" << report.diagnostics_count << " backend_reports=" << report.backend_reports_count
              << " scene_gpu_status="
              << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " scene_gpu_mesh_bindings=" << report.scene_gpu_stats.mesh_bindings
              << " scene_gpu_material_bindings=" << report.scene_gpu_stats.material_bindings
              << " scene_gpu_mesh_uploads=" << report.scene_gpu_stats.mesh_uploads
              << " scene_gpu_texture_uploads=" << report.scene_gpu_stats.texture_uploads
              << " scene_gpu_material_uploads=" << report.scene_gpu_stats.material_uploads
              << " scene_gpu_material_pipeline_layouts=" << report.scene_gpu_stats.material_pipeline_layouts
              << " scene_gpu_uploaded_texture_bytes=" << report.scene_gpu_stats.uploaded_texture_bytes
              << " scene_gpu_uploaded_mesh_bytes=" << report.scene_gpu_stats.uploaded_mesh_bytes
              << " scene_gpu_uploaded_material_factor_bytes=" << report.scene_gpu_stats.uploaded_material_factor_bytes
              << " scene_gpu_morph_mesh_bindings=" << report.scene_gpu_stats.morph_mesh_bindings
              << " scene_gpu_morph_mesh_uploads=" << report.scene_gpu_stats.morph_mesh_uploads
              << " scene_gpu_uploaded_morph_bytes=" << report.scene_gpu_stats.uploaded_morph_bytes
              << " scene_gpu_mesh_resolved=" << report.scene_gpu_stats.mesh_bindings_resolved
              << " scene_gpu_skinned_mesh_bindings=" << report.scene_gpu_stats.skinned_mesh_bindings
              << " scene_gpu_skinned_mesh_uploads=" << report.scene_gpu_stats.skinned_mesh_uploads
              << " scene_gpu_skinned_mesh_resolved=" << report.scene_gpu_stats.skinned_mesh_bindings_resolved
              << " scene_gpu_material_resolved=" << report.scene_gpu_stats.material_bindings_resolved
              << " postprocess_status="
              << mirakana::win32_desktop_presentation_postprocess_status_name(report.postprocess_status)
              << " postprocess_depth_input_requested=" << (report.postprocess_depth_input_requested ? 1 : 0)
              << " postprocess_depth_input_ready=" << (report.postprocess_depth_input_ready ? 1 : 0)
              << " postprocess_policy_status="
              << mirakana::win32_desktop_presentation_postprocess_policy_status_name(postprocess_policy.status)
              << " postprocess_policy_ready=" << (postprocess_policy.ready ? 1 : 0)
              << " postprocess_policy_diagnostics=" << postprocess_policy.diagnostics_count
              << " postprocess_policy_effects=" << postprocess_policy.effect_count
              << " postprocess_policy_postprocess_passes=" << postprocess_policy.postprocess_pass_count
              << " postprocess_policy_framegraph_passes=" << postprocess_policy.framegraph_pass_count
              << " postprocess_policy_framegraph_barrier_step_budget="
              << postprocess_policy.framegraph_barrier_step_budget
              << " postprocess_policy_scene_color_required=" << (postprocess_policy.scene_color_required ? 1 : 0)
              << " postprocess_policy_scene_depth_required=" << (postprocess_policy.scene_depth_required ? 1 : 0)
              << " postprocess_policy_color_grading_effect=" << (postprocess_policy.color_grading_effect ? 1 : 0)
              << " postprocess_policy_backend_shader_evidence_ready="
              << (postprocess_policy.backend_shader_evidence_ready ? 1 : 0) << " directional_shadow_status="
              << mirakana::win32_desktop_presentation_directional_shadow_status_name(report.directional_shadow_status)
              << " directional_shadow_requested=" << (report.directional_shadow_requested ? 1 : 0)
              << " directional_shadow_ready=" << (report.directional_shadow_ready ? 1 : 0)
              << " directional_shadow_filter_mode="
              << mirakana::win32_desktop_presentation_directional_shadow_filter_mode_name(
                     report.directional_shadow_filter_mode)
              << " directional_shadow_filter_taps=" << report.directional_shadow_filter_tap_count
              << " directional_shadow_filter_radius_texels=" << report.directional_shadow_filter_radius_texels
              << " directional_shadow_cascade_count=" << report.directional_shadow_cascade_count
              << " directional_shadow_cascade_tile_width=" << report.directional_shadow_cascade_tile_width
              << " directional_shadow_atlas_width=" << report.directional_shadow_atlas_width
              << " directional_shadow_atlas_height=" << report.directional_shadow_atlas_height
              << " directional_shadow_light_space_cascades=" << report.directional_shadow_light_space_cascades
              << " directional_shadow_cascade_splits=" << report.directional_shadow_cascade_splits
              << " ui_overlay_requested=" << (report.native_ui_overlay_requested ? 1 : 0) << " ui_overlay_status="
              << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
              << " ui_overlay_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
              << " ui_overlay_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
              << " ui_overlay_draws=" << report.native_ui_overlay_draws
              << " ui_texture_overlay_requested=" << (report.native_ui_texture_overlay_requested ? 1 : 0)
              << " ui_texture_overlay_status="
              << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(
                     report.native_ui_texture_overlay_status)
              << " ui_texture_overlay_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
              << " ui_texture_overlay_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
              << " ui_texture_overlay_texture_binds=" << report.native_ui_texture_overlay_texture_binds
              << " ui_texture_overlay_draws=" << report.native_ui_texture_overlay_draws
              << " framegraph_passes=" << report.framegraph_passes
              << " framegraph_passes_executed=" << report.renderer_stats.framegraph_passes_executed
              << " framegraph_render_passes_recorded=" << report.renderer_stats.framegraph_render_passes_recorded
              << " framegraph_barrier_steps_executed=" << report.renderer_stats.framegraph_barrier_steps_executed
              << " renderer_gpu_skinning_draws=" << report.renderer_stats.gpu_skinning_draws
              << " renderer_skinned_palette_descriptor_binds=" << report.renderer_stats.skinned_palette_descriptor_binds
              << " rhi_instanced_draw_calls=" << report.rhi_instanced_draw_calls
              << " rhi_instanced_indexed_draw_calls=" << report.rhi_instanced_indexed_draw_calls
              << " rhi_instanced_instances_submitted=" << report.rhi_instanced_instances_submitted
              << " renderer_frames_finished=" << report.renderer_stats.frames_finished << '\n';
    for (const auto& backend_report : host.presentation_backend_reports()) {
        std::cout << prefix << " presentation_backend_report="
                  << mirakana::win32_desktop_presentation_backend_name(backend_report.backend) << ":"
                  << mirakana::win32_desktop_presentation_backend_report_status_name(backend_report.status) << ":"
                  << mirakana::win32_desktop_presentation_fallback_reason_name(backend_report.fallback_reason) << ": "
                  << backend_report.diagnostic << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    DesktopRuntimeGameOptions options;
    if (!parse_args(argc, argv, options)) {
        print_usage();
        return 2;
    }
    if (options.show_help) {
        print_usage();
        return 0;
    }
    const bool require_environment_rain = options.require_environment_precipitation_package_evidence ||
                                          options.require_environment_precipitation_renderer_execution;
    const bool require_environment_snow =
        options.require_environment_snow_package_evidence || options.require_environment_snow_renderer_execution;
    if (require_environment_rain && require_environment_snow) {
        print_usage();
        return 2;
    }
    if (!verify_required_config(argc > 0 ? argv[0] : nullptr, options.required_config_path)) {
        return 4;
    }
    std::optional<mirakana::runtime::RuntimeAssetPackage> runtime_package;
    std::optional<mirakana::RuntimeSceneRenderInstance> packaged_scene;
    std::vector<mirakana::AnimationJointTrack3dDesc> packaged_quaternion_animation_tracks;
    if (!load_required_scene_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                     packaged_scene, packaged_quaternion_animation_tracks)) {
        return 4;
    }
    if (options.require_environment_profile && !runtime_package.has_value()) {
        std::cerr << "--require-environment-profile requires --require-scene-package\n";
        return 4;
    }
    if (options.require_scene_gpu_bindings && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-scene-gpu-bindings requires --require-scene-package\n";
        return 4;
    }
    if (options.require_gpu_skinning && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-gpu-skinning requires --require-scene-package\n";
        return 4;
    }
    if (options.require_lighting_shadow_policy && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-lighting-shadow-policy requires --require-scene-package\n";
        return 4;
    }
    if (options.require_quaternion_animation && packaged_quaternion_animation_tracks.empty()) {
        std::cerr << "--require-quaternion-animation requires --require-scene-package\n";
        return 4;
    }
    UiAtlasMetadataRuntimeState ui_atlas_metadata;
    if (options.require_native_ui_textured_sprite_atlas) {
        ui_atlas_metadata = load_required_ui_atlas_metadata(runtime_package.has_value() ? &*runtime_package : nullptr);
        if (ui_atlas_metadata.status != UiAtlasMetadataStatus::ready) {
            std::cout << "sample_desktop_runtime_game required_ui_atlas_metadata_unavailable status="
                      << ui_atlas_metadata_status_name(ui_atlas_metadata.status) << " pages=" << ui_atlas_metadata.pages
                      << " bindings=" << ui_atlas_metadata.bindings << '\n';
            print_ui_atlas_metadata_diagnostics("sample_desktop_runtime_game", ui_atlas_metadata);
            return 4;
        }
    }

    auto shader_bytecode = load_packaged_d3d12_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shader_bytecode.ready()) {
        std::cout << "sample_desktop_runtime_game shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shader_bytecode.status) << ": "
                  << shader_bytecode.diagnostic << '\n';
        if (options.require_d3d12_scene_shaders) {
            return 4;
        }
    }
    auto postprocess_bytecode = load_packaged_d3d12_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!postprocess_bytecode.ready()) {
        std::cout << "sample_desktop_runtime_game postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(postprocess_bytecode.status) << ": "
                  << postprocess_bytecode.diagnostic << '\n';
        if (options.require_postprocess && !options.require_vulkan_renderer) {
            return 4;
        }
    }
    auto environment_ibl_sample_bytecode =
        load_packaged_d3d12_environment_ibl_sample_shaders(argc > 0 ? argv[0] : nullptr);
    if (!environment_ibl_sample_bytecode.ready() && options.require_environment_lighting_renderer_execution) {
        std::cout << "sample_desktop_runtime_game d3d12_environment_ibl_sample_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(environment_ibl_sample_bytecode.status) << ": "
                  << environment_ibl_sample_bytecode.diagnostic << '\n';
        return 4;
    }
    auto environment_ibl_sample_vulkan_bytecode =
        load_packaged_vulkan_environment_ibl_sample_shaders(argc > 0 ? argv[0] : nullptr);
    if (!environment_ibl_sample_vulkan_bytecode.ready() &&
        options.require_environment_lighting_vulkan_renderer_execution) {
        std::cout << "sample_desktop_runtime_game vulkan_environment_ibl_sample_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(environment_ibl_sample_vulkan_bytecode.status)
                  << ": " << environment_ibl_sample_vulkan_bytecode.diagnostic << '\n';
        return 4;
    }
    auto cloud_layer_bytecode = load_packaged_d3d12_cloud_layer_shaders(argc > 0 ? argv[0] : nullptr);
    if (!cloud_layer_bytecode.ready() && options.require_cloud_layer_renderer_execution) {
        std::cout << "sample_desktop_runtime_game d3d12_cloud_layer_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(cloud_layer_bytecode.status) << ": "
                  << cloud_layer_bytecode.diagnostic << '\n';
        return 4;
    }
    auto precipitation_bytecode = load_packaged_d3d12_precipitation_shaders(argc > 0 ? argv[0] : nullptr);
    if (!precipitation_bytecode.ready() && (options.require_environment_precipitation_renderer_execution ||
                                            options.require_environment_snow_renderer_execution)) {
        std::cout << "sample_desktop_runtime_game d3d12_precipitation_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(precipitation_bytecode.status) << ": "
                  << precipitation_bytecode.diagnostic << '\n';
        return 4;
    }
    auto volumetric_cloud_bytecode = load_packaged_d3d12_volumetric_cloud_shaders(argc > 0 ? argv[0] : nullptr);
    if (!volumetric_cloud_bytecode.ready() && options.require_environment_volumetric_cloud_renderer_execution) {
        std::cout << "sample_desktop_runtime_game d3d12_volumetric_cloud_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(volumetric_cloud_bytecode.status) << ": "
                  << volumetric_cloud_bytecode.diagnostic << '\n';
        return 4;
    }
    const auto environment_fog_fragment_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeEnvironmentFogFragmentShaderPath, "height_fog_ps_main");
    if (options.require_environment_fog_evidence && !options.require_vulkan_renderer &&
        environment_fog_fragment_blob.bytecode.empty()) {
        std::cout << "sample_desktop_runtime_game environment_fog_shader_diagnostic=missing: "
                  << kRuntimeEnvironmentFogFragmentShaderPath << '\n';
        return 4;
    }
    const auto environment_volumetric_fog_compute_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeEnvironmentVolumetricFogComputeShaderPath, "main");
    if (options.require_environment_volumetric_fog_package_evidence &&
        environment_volumetric_fog_compute_blob.bytecode.empty()) {
        std::cout << "sample_desktop_runtime_game environment_volumetric_fog_shader_diagnostic=missing: "
                  << kRuntimeEnvironmentVolumetricFogComputeShaderPath << '\n';
        return 4;
    }
    auto shadow_receiver_bytecode = load_packaged_d3d12_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shadow_receiver_bytecode.ready() && options.require_directional_shadow && !options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shadow_receiver_bytecode.status) << ": "
                  << shadow_receiver_bytecode.diagnostic << '\n';
        return 4;
    }
    auto shadow_bytecode = load_packaged_d3d12_shadow_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shadow_bytecode.ready() && options.require_directional_shadow && !options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game shadow_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shadow_bytecode.status) << ": "
                  << shadow_bytecode.diagnostic << '\n';
        return 4;
    }
    auto native_ui_overlay_bytecode = load_packaged_d3d12_native_ui_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!native_ui_overlay_bytecode.ready() && options.require_native_ui_overlay && !options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game native_ui_overlay_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(native_ui_overlay_bytecode.status) << ": "
                  << native_ui_overlay_bytecode.diagnostic << '\n';
        return 4;
    }

    const auto d3d12_skinned_vertex_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeSceneSkinnedVertexShaderPath, "vs_skinned");
    const auto vulkan_skinned_vertex_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeSceneSkinnedVulkanVertexShaderPath, "vs_skinned");
    if (options.require_gpu_skinning) {
        if (!options.require_vulkan_renderer && d3d12_skinned_vertex_blob.bytecode.empty()) {
            std::cout << "sample_desktop_runtime_game required_gpu_skinning_missing_skinned_d3d12_vs\n";
            return 4;
        }
        if (options.require_vulkan_renderer && vulkan_skinned_vertex_blob.bytecode.empty()) {
            std::cout << "sample_desktop_runtime_game required_gpu_skinning_missing_skinned_vulkan_vs\n";
            return 6;
        }
    }

    const auto d3d12_skinned_shadow_receiver_fragment_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeShadowReceiverSkinnedFragmentShaderPath, "ps_shadow_receiver");
    const auto vulkan_skinned_shadow_receiver_fragment_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeShadowReceiverSkinnedVulkanFragmentShaderPath, "ps_shadow_receiver");
    if (options.require_directional_shadow && options.require_gpu_skinning) {
        if (!options.require_vulkan_renderer && d3d12_skinned_shadow_receiver_fragment_blob.bytecode.empty()) {
            std::cout << "sample_desktop_runtime_game required_skinned_shadow_receiver_missing_d3d12_ps\n";
            return 4;
        }
        if (options.require_vulkan_renderer && vulkan_skinned_shadow_receiver_fragment_blob.bytecode.empty()) {
            std::cout << "sample_desktop_runtime_game required_skinned_shadow_receiver_missing_vulkan_ps\n";
            return 6;
        }
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shader_bytecode.ready()) {
        if (options.require_vulkan_scene_shaders) {
            std::cout << "sample_desktop_runtime_game vulkan_shader_diagnostic="
                      << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shader_bytecode.status) << ": "
                      << vulkan_shader_bytecode.diagnostic << '\n';
            return 6;
        }
    }
    auto vulkan_postprocess_bytecode = load_packaged_vulkan_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (options.require_environment_fog_vulkan_package_evidence) {
        vulkan_postprocess_bytecode = load_packaged_vulkan_environment_fog_shaders(argc > 0 ? argv[0] : nullptr);
    }
    if (!vulkan_postprocess_bytecode.ready()) {
        if (options.require_postprocess && options.require_vulkan_renderer) {
            std::cout << "sample_desktop_runtime_game vulkan_postprocess_shader_diagnostic="
                      << mirakana::desktop_shader_bytecode_load_status_name(vulkan_postprocess_bytecode.status) << ": "
                      << vulkan_postprocess_bytecode.diagnostic << '\n';
            return 6;
        }
    }
    auto vulkan_precipitation_bytecode = load_packaged_vulkan_precipitation_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_precipitation_bytecode.ready() && options.require_environment_precipitation_vulkan_renderer_execution) {
        std::cout << "sample_desktop_runtime_game vulkan_precipitation_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_precipitation_bytecode.status) << ": "
                  << vulkan_precipitation_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_volumetric_cloud_bytecode = load_packaged_vulkan_volumetric_cloud_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_volumetric_cloud_bytecode.ready() &&
        options.require_environment_volumetric_cloud_vulkan_renderer_execution) {
        std::cout << "sample_desktop_runtime_game vulkan_environment_volumetric_cloud_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_volumetric_cloud_bytecode.status) << ": "
                  << vulkan_volumetric_cloud_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shadow_receiver_bytecode =
        load_packaged_vulkan_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shadow_receiver_bytecode.ready() && options.require_directional_shadow &&
        options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game vulkan_shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shadow_receiver_bytecode.status) << ": "
                  << vulkan_shadow_receiver_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shadow_bytecode = load_packaged_vulkan_shadow_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shadow_bytecode.ready() && options.require_directional_shadow && options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game vulkan_shadow_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shadow_bytecode.status) << ": "
                  << vulkan_shadow_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_native_ui_overlay_bytecode =
        load_packaged_vulkan_native_ui_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_native_ui_overlay_bytecode.ready() && options.require_native_ui_overlay &&
        options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game vulkan_native_ui_overlay_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_native_ui_overlay_bytecode.status)
                  << ": " << vulkan_native_ui_overlay_bytecode.diagnostic << '\n';
        return 6;
    }
    const auto vulkan_compute_mapping_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeSceneVulkanComputeMappingShaderPath, "cs_vulkan_mapping_proof");
    if (vulkan_compute_mapping_blob.bytecode.empty() && options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game vulkan_compute_mapping_shader_diagnostic=missing: "
                  << kRuntimeSceneVulkanComputeMappingShaderPath << '\n';
        return 6;
    }
    const auto environment_volumetric_fog_vulkan_compute_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeEnvironmentVolumetricFogVulkanComputeShaderPath, "main");
    if (options.require_environment_volumetric_fog_vulkan_renderer_execution &&
        environment_volumetric_fog_vulkan_compute_blob.bytecode.empty()) {
        std::cout << "sample_desktop_runtime_game vulkan_environment_volumetric_fog_shader_diagnostic=missing: "
                  << kRuntimeEnvironmentVolumetricFogVulkanComputeShaderPath << '\n';
        return 6;
    }

    const bool require_environment_any_precipitation_package_evidence =
        options.require_environment_precipitation_package_evidence ||
        options.require_environment_snow_package_evidence ||
        options.require_environment_precipitation_renderer_execution ||
        options.require_environment_snow_renderer_execution;
    const bool require_environment_snow_precipitation =
        options.require_environment_snow_package_evidence || options.require_environment_snow_renderer_execution;
    const bool require_environment_precipitation_renderer_execution =
        options.require_environment_precipitation_renderer_execution ||
        options.require_environment_snow_renderer_execution;
    const bool require_environment_volumetric_cloud = options.require_environment_volumetric_cloud_package_evidence ||
                                                      options.require_environment_volumetric_cloud_renderer_execution;
    const mirakana::Win32DesktopPresentationEnvironmentPrecipitationExpectation environment_precipitation_expectation{
        .weather = require_environment_snow_precipitation ? mirakana::EnvironmentWeatherKind::snow
                                                          : mirakana::EnvironmentWeatherKind::storm,
        .kind = require_environment_snow_precipitation ? mirakana::EnvironmentPrecipitationKind::snow
                                                       : mirakana::EnvironmentPrecipitationKind::rain,
        .wetness_rows = require_environment_snow_precipitation ? 0U : 1U,
        .minimum_audio_handoff_rows = require_environment_snow_precipitation ? 1U : 4U,
        .require_renderer_execution = require_environment_precipitation_renderer_execution,
    };
    const mirakana::Win32DesktopPresentationEnvironmentPrecipitationExpectation
        environment_precipitation_vulkan_expectation{
            .weather = mirakana::EnvironmentWeatherKind::storm,
            .kind = mirakana::EnvironmentPrecipitationKind::rain,
            .wetness_rows = 1U,
            .minimum_audio_handoff_rows = 4U,
            .require_renderer_execution = options.require_environment_precipitation_vulkan_renderer_execution,
        };
    std::optional<mirakana::Win32DesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;
    const auto& d3d12_scene_bytecode = options.require_directional_shadow ? shadow_receiver_bytecode : shader_bytecode;
    const bool d3d12_shadow_ready = !options.require_directional_shadow || shadow_bytecode.ready();
    const bool d3d12_native_ui_overlay_ready = !options.require_native_ui_overlay || native_ui_overlay_bytecode.ready();
    const auto* d3d12_postprocess_fragment_shader = &postprocess_bytecode.fragment_shader;
    if (options.require_environment_fog_evidence) {
        d3d12_postprocess_fragment_shader = &environment_fog_fragment_blob;
    }
    if (d3d12_scene_bytecode.ready() && postprocess_bytecode.ready() && d3d12_shadow_ready &&
        d3d12_native_ui_overlay_ready && runtime_package.has_value() && packaged_scene.has_value()) {
        d3d12_scene_renderer.emplace(mirakana::Win32DesktopPresentationD3d12SceneRendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(d3d12_scene_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(d3d12_scene_bytecode.fragment_shader),
            .skinned_vertex_shader = to_presentation_shader_bytecode(d3d12_skinned_vertex_blob),
            .skinned_scene_fragment_shader =
                to_presentation_shader_bytecode(d3d12_skinned_shadow_receiver_fragment_blob),
            .postprocess_vertex_shader = to_presentation_shader_bytecode(postprocess_bytecode.vertex_shader),
            .postprocess_fragment_shader = to_presentation_shader_bytecode(*d3d12_postprocess_fragment_shader),
            .shadow_vertex_shader = to_presentation_shader_bytecode(shadow_bytecode.vertex_shader),
            .shadow_fragment_shader = to_presentation_shader_bytecode(shadow_bytecode.fragment_shader),
            .native_ui_overlay_vertex_shader =
                to_presentation_shader_bytecode(native_ui_overlay_bytecode.vertex_shader),
            .native_ui_overlay_fragment_shader =
                to_presentation_shader_bytecode(native_ui_overlay_bytecode.fragment_shader),
            .cloud_layer_vertex_shader = to_presentation_shader_bytecode(cloud_layer_bytecode.vertex_shader),
            .cloud_layer_fragment_shader = to_presentation_shader_bytecode(cloud_layer_bytecode.fragment_shader),
            .precipitation_vertex_shader = to_presentation_shader_bytecode(precipitation_bytecode.vertex_shader),
            .precipitation_fragment_shader = to_presentation_shader_bytecode(precipitation_bytecode.fragment_shader),
            .volumetric_cloud_vertex_shader = to_presentation_shader_bytecode(volumetric_cloud_bytecode.vertex_shader),
            .volumetric_cloud_fragment_shader =
                to_presentation_shader_bytecode(volumetric_cloud_bytecode.fragment_shader),
            .package = &*runtime_package,
            .packet = &packaged_scene->render_packet,
            .vertex_buffers = runtime_scene_vertex_buffers(),
            .vertex_attributes = runtime_scene_vertex_attributes(),
            .skinned_vertex_buffers = runtime_skinned_scene_vertex_buffers(),
            .skinned_vertex_attributes = runtime_skinned_scene_vertex_attributes(),
            .enable_postprocess = true,
            .enable_postprocess_depth_input = true,
            .enable_environment_fog = options.require_environment_fog_evidence,
            .environment_fog = make_sample_environment_fog_policy_desc(),
            .enable_physical_sky_package_evidence = options.require_physical_sky_package_evidence,
            .physical_sky = make_sample_physical_sky_policy_desc(),
            .enable_cloud_layer_package_evidence = options.require_cloud_layer_package_evidence,
            .enable_cloud_layer_renderer_execution = options.require_cloud_layer_renderer_execution,
            .cloud_layer = make_sample_cloud_layer_policy_desc(options.require_cloud_layer_renderer_execution),
            .enable_environment_precipitation_package_evidence = require_environment_any_precipitation_package_evidence,
            .enable_environment_precipitation_renderer_execution = require_environment_precipitation_renderer_execution,
            .environment_precipitation = require_environment_snow_precipitation
                                             ? make_sample_environment_snow_policy_desc()
                                             : make_sample_environment_precipitation_policy_desc(),
            .enable_environment_volumetric_fog_package_evidence =
                options.require_environment_volumetric_fog_package_evidence,
            .environment_volumetric_fog = make_sample_environment_volumetric_fog_policy_desc(
                !environment_volumetric_fog_compute_blob.bytecode.empty()),
            .enable_environment_volumetric_cloud_package_evidence = require_environment_volumetric_cloud,
            .enable_environment_volumetric_cloud_renderer_execution =
                options.require_environment_volumetric_cloud_renderer_execution,
            .environment_volumetric_cloud = make_sample_environment_volumetric_cloud_policy_desc(),
            .enable_directional_shadow_smoke = options.require_directional_shadow,
            .enable_native_ui_overlay = options.require_native_ui_overlay,
            .native_ui_overlay_atlas_asset =
                options.require_native_ui_textured_sprite_atlas ? ui_atlas_metadata.atlas_page : mirakana::AssetId{},
            .enable_native_ui_overlay_textures = options.require_native_ui_textured_sprite_atlas,
        });
    }

    std::optional<mirakana::Win32DesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;
    const auto& vulkan_scene_bytecode =
        options.require_directional_shadow ? vulkan_shadow_receiver_bytecode : vulkan_shader_bytecode;
    const bool vulkan_shadow_ready = !options.require_directional_shadow || vulkan_shadow_bytecode.ready();
    const bool vulkan_native_ui_overlay_ready =
        !options.require_native_ui_overlay || vulkan_native_ui_overlay_bytecode.ready();
    const bool vulkan_compute_mapping_ready = !vulkan_compute_mapping_blob.bytecode.empty();
    const bool vulkan_volumetric_cloud_ready =
        !options.require_environment_volumetric_cloud_vulkan_renderer_execution ||
        vulkan_volumetric_cloud_bytecode.ready();
    if (vulkan_scene_bytecode.ready() && vulkan_postprocess_bytecode.ready() && vulkan_shadow_ready &&
        vulkan_native_ui_overlay_ready && vulkan_compute_mapping_ready && vulkan_volumetric_cloud_ready &&
        runtime_package.has_value() && packaged_scene.has_value()) {
        vulkan_scene_renderer.emplace(mirakana::Win32DesktopPresentationVulkanSceneRendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(vulkan_scene_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(vulkan_scene_bytecode.fragment_shader),
            .skinned_vertex_shader = to_presentation_shader_bytecode(vulkan_skinned_vertex_blob),
            .compute_morph_shader = to_presentation_shader_bytecode(vulkan_compute_mapping_blob),
            .skinned_scene_fragment_shader =
                to_presentation_shader_bytecode(vulkan_skinned_shadow_receiver_fragment_blob),
            .postprocess_vertex_shader = to_presentation_shader_bytecode(vulkan_postprocess_bytecode.vertex_shader),
            .postprocess_fragment_shader = to_presentation_shader_bytecode(vulkan_postprocess_bytecode.fragment_shader),
            .shadow_vertex_shader = to_presentation_shader_bytecode(vulkan_shadow_bytecode.vertex_shader),
            .shadow_fragment_shader = to_presentation_shader_bytecode(vulkan_shadow_bytecode.fragment_shader),
            .native_ui_overlay_vertex_shader =
                to_presentation_shader_bytecode(vulkan_native_ui_overlay_bytecode.vertex_shader),
            .native_ui_overlay_fragment_shader =
                to_presentation_shader_bytecode(vulkan_native_ui_overlay_bytecode.fragment_shader),
            .precipitation_vertex_shader = to_presentation_shader_bytecode(vulkan_precipitation_bytecode.vertex_shader),
            .precipitation_fragment_shader =
                to_presentation_shader_bytecode(vulkan_precipitation_bytecode.fragment_shader),
            .volumetric_fog_compute_shader =
                to_presentation_shader_bytecode(environment_volumetric_fog_vulkan_compute_blob),
            .volumetric_cloud_vertex_shader =
                to_presentation_shader_bytecode(vulkan_volumetric_cloud_bytecode.vertex_shader),
            .volumetric_cloud_fragment_shader =
                to_presentation_shader_bytecode(vulkan_volumetric_cloud_bytecode.fragment_shader),
            .package = &*runtime_package,
            .packet = &packaged_scene->render_packet,
            .vertex_buffers = runtime_scene_vertex_buffers(),
            .vertex_attributes = runtime_scene_vertex_attributes(),
            .skinned_vertex_buffers = runtime_skinned_scene_vertex_buffers(),
            .skinned_vertex_attributes = runtime_skinned_scene_vertex_attributes(),
            .enable_postprocess = true,
            .enable_postprocess_depth_input = true,
            .enable_environment_fog = options.require_environment_fog_vulkan_package_evidence,
            .environment_fog =
                [] {
                    auto desc = make_sample_environment_fog_policy_desc();
                    desc.package_evidence_ready = true;
                    return desc;
                }(),
            .enable_physical_sky_package_evidence = options.require_physical_sky_vulkan_package_evidence,
            .physical_sky = make_sample_physical_sky_policy_desc(),
            .enable_environment_precipitation_package_evidence =
                options.require_environment_precipitation_vulkan_renderer_execution,
            .enable_environment_precipitation_renderer_execution =
                options.require_environment_precipitation_vulkan_renderer_execution,
            .environment_precipitation = make_sample_environment_precipitation_policy_desc(),
            .enable_environment_volumetric_fog_renderer_execution =
                options.require_environment_volumetric_fog_vulkan_renderer_execution,
            .environment_volumetric_fog = make_sample_environment_volumetric_fog_policy_desc(
                !environment_volumetric_fog_vulkan_compute_blob.bytecode.empty()),
            .enable_environment_volumetric_cloud_renderer_execution =
                options.require_environment_volumetric_cloud_vulkan_renderer_execution,
            .environment_volumetric_cloud = make_sample_environment_volumetric_cloud_policy_desc(),
            .enable_directional_shadow_smoke = options.require_directional_shadow,
            .enable_native_ui_overlay = options.require_native_ui_overlay,
            .native_ui_overlay_atlas_asset =
                options.require_native_ui_textured_sprite_atlas ? ui_atlas_metadata.atlas_page : mirakana::AssetId{},
            .enable_native_ui_overlay_textures = options.require_native_ui_textured_sprite_atlas,
            .enable_strict_validation = options.require_environment_vulkan_strict_aggregate,
        });
    }

    mirakana::Win32DesktopGameHostDesc host_desc{
        .title = "Sample Desktop Runtime Game",
        .extent = mirakana::WindowExtent{.width = 800, .height = 450},
        .prefer_vulkan = options.require_vulkan_renderer,
    };
    if (d3d12_scene_renderer.has_value()) {
        host_desc.d3d12_scene_renderer = &*d3d12_scene_renderer;
    }
    if (vulkan_scene_renderer.has_value()) {
        host_desc.vulkan_scene_renderer = &*vulkan_scene_renderer;
    }

    mirakana::Win32DesktopGameHost host(host_desc);
    if (options.require_d3d12_renderer &&
        host.presentation_backend() != mirakana::Win32DesktopPresentationBackend::d3d12) {
        std::cout << "sample_desktop_runtime_game required_d3d12_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 5;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::Win32DesktopPresentationBackend::vulkan) {
        std::cout << "sample_desktop_runtime_game required_vulkan_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 7;
    }
    if (options.require_scene_gpu_bindings && !host.scene_gpu_bindings_ready()) {
        std::cout << "sample_desktop_runtime_game required_scene_gpu_bindings_unavailable status="
                  << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(host.scene_gpu_binding_status())
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 5;
    }
    if (options.require_postprocess &&
        host.presentation_report().postprocess_status != mirakana::Win32DesktopPresentationPostprocessStatus::ready) {
        std::cout << "sample_desktop_runtime_game required_postprocess_unavailable status="
                  << mirakana::win32_desktop_presentation_postprocess_status_name(
                         host.presentation_report().postprocess_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.postprocess_diagnostics()) {
            std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                      << mirakana::win32_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 8;
    }
    if (options.require_postprocess_depth_input && !host.presentation_report().postprocess_depth_input_ready) {
        std::cout << "sample_desktop_runtime_game required_postprocess_depth_input_unavailable ready=0\n";
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.postprocess_diagnostics()) {
            std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                      << mirakana::win32_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 8;
    }
    if (options.require_directional_shadow && !host.presentation_report().directional_shadow_ready) {
        std::cout << "sample_desktop_runtime_game required_directional_shadow_unavailable status="
                  << mirakana::win32_desktop_presentation_directional_shadow_status_name(
                         host.presentation_report().directional_shadow_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.directional_shadow_diagnostics()) {
            std::cout << "sample_desktop_runtime_game directional_shadow_diagnostic="
                      << mirakana::win32_desktop_presentation_directional_shadow_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 9;
    }
    if (options.require_directional_shadow_filtering) {
        const auto report = host.presentation_report();
        if (report.directional_shadow_filter_mode !=
                mirakana::Win32DesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||
            report.directional_shadow_filter_tap_count != 9 || report.directional_shadow_filter_radius_texels != 1.0F) {
            std::cout << "sample_desktop_runtime_game required_directional_shadow_filtering_unavailable mode="
                      << mirakana::win32_desktop_presentation_directional_shadow_filter_mode_name(
                             report.directional_shadow_filter_mode)
                      << " taps=" << report.directional_shadow_filter_tap_count
                      << " radius_texels=" << report.directional_shadow_filter_radius_texels << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            return 9;
        }
    }
    if (options.require_d3d12_shadow_cascade_policy) {
        const auto report = host.presentation_report();
        if (!directional_shadow_cascade_policy_matches(report)) {
            std::cout << "sample_desktop_runtime_game required_d3d12_shadow_cascade_policy_unavailable"
                      << " cascades=" << report.directional_shadow_cascade_count
                      << " tile_width=" << report.directional_shadow_cascade_tile_width
                      << " atlas_width=" << report.directional_shadow_atlas_width
                      << " atlas_height=" << report.directional_shadow_atlas_height
                      << " light_space_cascades=" << report.directional_shadow_light_space_cascades
                      << " cascade_splits=" << report.directional_shadow_cascade_splits << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            return 9;
        }
    }
    if (options.require_vulkan_shadow_cascade_policy) {
        const auto report = host.presentation_report();
        if (!vulkan_shadow_cascade_policy_ready(report)) {
            std::cout << "sample_desktop_runtime_game required_vulkan_shadow_cascade_policy_unavailable"
                      << " renderer=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
                      << " cascades=" << report.directional_shadow_cascade_count
                      << " tile_width=" << report.directional_shadow_cascade_tile_width
                      << " atlas_width=" << report.directional_shadow_atlas_width
                      << " atlas_height=" << report.directional_shadow_atlas_height
                      << " light_space_cascades=" << report.directional_shadow_light_space_cascades
                      << " cascade_splits=" << report.directional_shadow_cascade_splits << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            return 9;
        }
    }
    if (options.require_native_ui_overlay && !host.presentation_report().native_ui_overlay_ready) {
        std::cout << "sample_desktop_runtime_game required_native_ui_overlay_unavailable status="
                  << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(
                         host.presentation_report().native_ui_overlay_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.native_ui_overlay_diagnostics()) {
            std::cout << "sample_desktop_runtime_game native_ui_overlay_diagnostic="
                      << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 10;
    }
    if (options.require_native_ui_textured_sprite_atlas) {
        const auto report = host.presentation_report();
        if (report.native_ui_texture_overlay_status !=
                mirakana::Win32DesktopPresentationNativeUiTextureOverlayStatus::ready ||
            !report.native_ui_texture_overlay_atlas_ready) {
            std::cout << "sample_desktop_runtime_game required_native_ui_textured_sprite_atlas_unavailable status="
                      << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(
                             report.native_ui_texture_overlay_status)
                      << " atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0) << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            for (const auto& diagnostic : host.native_ui_texture_overlay_diagnostics()) {
                std::cout << "sample_desktop_runtime_game native_ui_texture_overlay_diagnostic="
                          << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(
                                 diagnostic.status)
                          << ": " << diagnostic.message << '\n';
            }
            return 11;
        }
    }

    SampleDesktopRuntimeGame game(host.input(), host.renderer(), options.throttle, std::move(packaged_scene),
                                  host.scene_gpu_bindings_ready(), std::move(packaged_quaternion_animation_tracks),
                                  options.require_lighting_shadow_policy,
                                  options.require_native_ui_textured_sprite_atlas, std::move(ui_atlas_metadata.palette),
                                  scene_mesh_instance_count(options), &host.presentation());
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();
    const auto scene_gpu_stats = report.scene_gpu_stats;
    const auto postprocess_policy = mirakana::evaluate_win32_desktop_presentation_postprocess_policy(report);
    const auto d3d12_postprocess_execution = mirakana::evaluate_win32_desktop_presentation_d3d12_postprocess_execution(
        report, static_cast<std::uint64_t>(options.max_frames), options.require_d3d12_postprocess_evidence);
    const auto environment_fog = mirakana::evaluate_win32_desktop_presentation_environment_fog(
        report, d3d12_postprocess_execution, options.require_environment_fog_evidence);
    const auto physical_sky = mirakana::evaluate_win32_desktop_presentation_physical_sky(
        report, options.require_physical_sky_package_evidence);
    const auto physical_sky_vulkan_package = mirakana::evaluate_win32_desktop_presentation_vulkan_physical_sky_package(
        report, options.require_physical_sky_vulkan_package_evidence);
    const auto cloud_layer = mirakana::evaluate_win32_desktop_presentation_cloud_layer(
        report, options.require_cloud_layer_package_evidence, options.require_cloud_layer_renderer_execution);
    const auto environment_precipitation = mirakana::evaluate_win32_desktop_presentation_environment_precipitation(
        report, require_environment_any_precipitation_package_evidence, environment_precipitation_expectation);
    const auto environment_precipitation_vulkan =
        mirakana::evaluate_win32_desktop_presentation_vulkan_environment_precipitation(
            report, options.require_environment_precipitation_vulkan_renderer_execution,
            environment_precipitation_vulkan_expectation);
    const auto environment_volumetric_fog = mirakana::evaluate_win32_desktop_presentation_environment_volumetric_fog(
        report, options.require_environment_volumetric_fog_package_evidence);
    const auto environment_volumetric_fog_vulkan =
        mirakana::evaluate_win32_desktop_presentation_vulkan_environment_volumetric_fog(
            report, options.require_environment_volumetric_fog_vulkan_renderer_execution);
    const auto environment_volumetric_cloud =
        mirakana::evaluate_win32_desktop_presentation_environment_volumetric_cloud(
            report, require_environment_volumetric_cloud,
            options.require_environment_volumetric_cloud_renderer_execution);
    const auto environment_volumetric_cloud_vulkan =
        mirakana::evaluate_win32_desktop_presentation_vulkan_environment_volumetric_cloud(
            report, options.require_environment_volumetric_cloud_vulkan_renderer_execution);
    const bool environment_quality_budget_required = environment_quality_budget_requested(options);
    const auto environment_profile = evaluate_environment_profile_package(
        options.require_environment_profile || environment_quality_budget_required, runtime_package);
    const auto environment_texture_asset_pipeline = evaluate_environment_texture_asset_pipeline_package(
        options.require_environment_texture_asset_pipeline_package, runtime_package);
    const auto environment_preset_library = evaluate_environment_preset_library_package(
        options.require_environment_preset_library_package, runtime_package);
    const auto environment_ibl_renderer_execution =
        mirakana::evaluate_win32_desktop_presentation_environment_ibl_renderer_execution(
            mirakana::Win32DesktopPresentationEnvironmentIblRendererExecutionDesc{
                .requested = options.require_environment_lighting_renderer_execution,
                .d3d12_backend_selected = report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12,
                .sampling_vertex_shader =
                    to_presentation_shader_bytecode(environment_ibl_sample_bytecode.vertex_shader),
                .sampling_fragment_shader =
                    to_presentation_shader_bytecode(environment_ibl_sample_bytecode.fragment_shader),
                .edge_size = 16,
                .mip_count = 5,
            });
    const auto environment_ibl_vulkan_renderer_execution =
        mirakana::evaluate_win32_desktop_presentation_vulkan_environment_ibl_renderer_execution(
            mirakana::Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionDesc{
                .requested = options.require_environment_lighting_vulkan_renderer_execution,
                .vulkan_backend_selected = report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan,
                .sampling_vertex_shader =
                    to_presentation_shader_bytecode(environment_ibl_sample_vulkan_bytecode.vertex_shader),
                .sampling_fragment_shader =
                    to_presentation_shader_bytecode(environment_ibl_sample_vulkan_bytecode.fragment_shader),
                .edge_size = 16,
                .mip_count = 5,
            });
    const auto environment_lighting = evaluate_environment_lighting_package(
        options.require_environment_lighting_package_evidence, options.require_environment_lighting_renderer_execution,
        environment_ibl_renderer_execution, runtime_package);
    const auto environment_material_weathering = evaluate_environment_material_weathering(
        options.require_environment_material_weathering, report, runtime_package);
    const auto environment_audio_playback =
        evaluate_environment_audio_playback(options.require_environment_audio_playback);
    const auto vulkan_postprocess_execution =
        mirakana::evaluate_win32_desktop_presentation_vulkan_postprocess_execution(
            report, static_cast<std::uint64_t>(options.max_frames), options.require_vulkan_postprocess_evidence);
    const auto environment_fog_vulkan_package =
        mirakana::evaluate_win32_desktop_presentation_vulkan_environment_fog_package(
            report, vulkan_postprocess_execution, options.require_environment_fog_vulkan_package_evidence);
    const auto d3d12_instanced_draw_execution =
        mirakana::evaluate_win32_desktop_presentation_d3d12_instanced_draw_execution(
            report, expected_d3d12_instanced_draw_instances(options));
    const auto vulkan_instanced_draw_execution =
        mirakana::evaluate_win32_desktop_presentation_vulkan_instanced_draw_execution(
            report, expected_vulkan_instanced_draw_instances(options));
    const auto scene_scale_policy = mirakana::evaluate_win32_desktop_presentation_scene_scale_policy(
        report, make_scene_scale_policy_desc(options, d3d12_instanced_draw_execution.ready ||
                                                          vulkan_instanced_draw_execution.ready));
    const auto d3d12_gpu_memory_execution = mirakana::evaluate_win32_desktop_presentation_d3d12_gpu_memory_execution(
        report, options.require_d3d12_gpu_memory_evidence);
    const auto vulkan_gpu_memory_execution = mirakana::evaluate_win32_desktop_presentation_vulkan_gpu_memory_execution(
        report, options.require_vulkan_gpu_memory_evidence);
    const auto gpu_memory_policy =
        mirakana::evaluate_win32_desktop_presentation_gpu_memory_policy(report, make_gpu_memory_policy_desc(options));
    const auto memory_diagnostics = summarize_package_memory_diagnostics(runtime_package, gpu_memory_policy);
    const auto d3d12_debug_profiling_execution =
        mirakana::evaluate_win32_desktop_presentation_d3d12_debug_profiling_execution(
            report, options.require_d3d12_debug_profiling_evidence);
    const auto vulkan_debug_profiling_execution =
        mirakana::evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution(
            report, options.require_vulkan_debug_profiling_evidence);
    const auto debug_profiling_policy = mirakana::evaluate_win32_desktop_presentation_debug_profiling_policy(
        report, make_debug_profiling_policy_desc(options));
    const auto job_scheduling_evidence = build_package_job_scheduling_evidence(options.require_job_scheduling_evidence);
    const auto job_execution_topology_policy =
        build_package_job_execution_topology_policy_evidence(options.require_job_execution_topology_policy);
    const auto job_execution_foundation =
        build_package_job_execution_foundation_evidence(options.require_job_execution_foundation);
    const auto job_execution_work_stealing =
        build_package_job_execution_work_stealing_evidence(options.require_job_execution_work_stealing);
    const auto job_execution_placement_policy =
        build_package_job_execution_placement_policy_evidence(options.require_job_execution_placement_policy);
    const auto windows_cpu_set_worker_placement = build_package_windows_cpu_set_worker_placement_evidence(
        options.require_windows_cpu_set_worker_placement,
        mirakana::JobExecutionPlacementPolicyMode::prefer_performance_cores, "package.windows_cpu_set");
    const auto windows_cpu_set_smt_worker_placement = build_package_windows_cpu_set_worker_placement_evidence(
        options.require_windows_cpu_set_smt_worker_placement,
        mirakana::JobExecutionPlacementPolicyMode::avoid_smt_siblings, "package.windows_cpu_set_smt");
    const auto simd_dispatch_policy = build_package_simd_dispatch_policy_evidence(options.require_simd_dispatch_policy);
    const auto renderer_quality =
        mirakana::evaluate_win32_desktop_presentation_quality_gate(report, make_renderer_quality_gate_desc(options));
    const bool framegraph_multiqueue_requested =
        options.require_framegraph_multiqueue_evidence || options.require_vulkan_framegraph_multiqueue_evidence;
    const auto framegraph_multiqueue = framegraph_multiqueue_requested
                                           ? run_frame_graph_multi_queue_package_evidence(host.presentation())
                                           : mirakana::FrameGraphRhiMultiQueuePackageEvidence{};
    const bool lighting_shadow_policy_ready = game.lighting_shadow_policy_passed(options.max_frames);
    const std::uint32_t expected_framegraph_passes = options.require_directional_shadow ? 3U : 2U;
    const auto expected_frames = static_cast<std::uint64_t>(options.max_frames);
    const auto expected_framegraph_render_passes = expected_frames * expected_framegraph_passes;
    const std::uint64_t expected_framegraph_barrier_steps =
        options.require_directional_shadow        ? (expected_frames == 0 ? 0 : 9 + ((expected_frames - 1) * 6))
        : options.require_postprocess_depth_input ? (expected_frames == 0 ? 0 : 1 + (expected_frames * 4))
                                                  : expected_frames * 2;
    const bool requires_any_environment_fog_postprocess =
        options.require_environment_fog_evidence || options.require_environment_fog_vulkan_package_evidence;
    const auto environment_quality_budget = build_environment_quality_budget_evidence(
        options, report, environment_fog, environment_fog_vulkan_package, physical_sky, physical_sky_vulkan_package,
        environment_lighting, environment_ibl_vulkan_renderer_execution, environment_material_weathering, cloud_layer,
        environment_precipitation, environment_precipitation_vulkan, environment_volumetric_fog,
        environment_volumetric_fog_vulkan, environment_volumetric_cloud, environment_volumetric_cloud_vulkan,
        environment_profile, environment_audio_playback,
        options.require_postprocess ? expected_framegraph_barrier_steps : 0U);
    const auto environment_ready_aggregate = build_environment_ready_aggregate_evidence(
        options, report, environment_fog, physical_sky, environment_lighting, environment_ibl_renderer_execution,
        environment_material_weathering, cloud_layer, environment_precipitation, environment_volumetric_fog,
        environment_volumetric_cloud, environment_profile, environment_audio_playback, environment_quality_budget);
    const auto environment_vulkan_strict_aggregate = build_environment_vulkan_strict_aggregate_evidence(
        options, report, vulkan_postprocess_execution, environment_fog_vulkan_package, physical_sky_vulkan_package,
        environment_ibl_vulkan_renderer_execution, environment_precipitation_vulkan, environment_volumetric_fog_vulkan,
        environment_volumetric_cloud_vulkan, environment_profile, environment_quality_budget);
    const auto environment_backend_parity =
        build_environment_backend_parity_smoke_evidence(options, environment_ready_aggregate);
    const auto environment_platform_readiness =
        build_environment_platform_readiness_smoke_evidence(options, environment_ready_aggregate);
    const auto environment_optimization_measurement =
        build_environment_optimization_measurement_smoke_evidence(options, environment_ready_aggregate);
    const auto environment_weather_simulation_package = build_environment_weather_simulation_package_evidence(options);

    std::cout
        << "sample_desktop_runtime_game status=" << status_name(result.status)
        << " renderer=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_requested=" << mirakana::win32_desktop_presentation_backend_name(report.requested_backend)
        << " presentation_selected=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_fallback="
        << mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason)
        << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
        << " presentation_backend_reports=" << report.backend_reports_count
        << " presentation_diagnostics=" << report.diagnostics_count << " frames=" << result.frames_run
        << " game_frames=" << game.frames() << " scene_meshes=" << game.scene_meshes_submitted()
        << " scene_materials=" << game.scene_materials_resolved()
        << " scene_mesh_plan_meshes=" << game.scene_mesh_plan_meshes()
        << " scene_mesh_plan_draws=" << game.scene_mesh_plan_draws()
        << " scene_mesh_plan_unique_meshes=" << game.scene_mesh_plan_unique_meshes()
        << " scene_mesh_plan_unique_materials=" << game.scene_mesh_plan_unique_materials()
        << " scene_mesh_plan_diagnostics=" << game.scene_mesh_plan_diagnostics()
        << " camera_primary=" << (game.primary_camera_seen() ? 1 : 0)
        << " camera_controller_ticks=" << game.camera_controller_ticks() << " camera_final_x=" << game.final_camera_x()
        << " quaternion_animation=" << (game.quaternion_animation_passed(options.max_frames) ? 1 : 0)
        << " quaternion_animation_ticks=" << game.quaternion_animation_ticks()
        << " quaternion_animation_tracks=" << game.quaternion_animation_tracks_sampled()
        << " quaternion_animation_failures=" << game.quaternion_animation_failures()
        << " quaternion_animation_scene_applied=" << game.quaternion_animation_scene_applied()
        << " quaternion_animation_final_z=" << game.final_quaternion_z()
        << " quaternion_animation_final_w=" << game.final_quaternion_w()
        << " quaternion_animation_scene_rotation_z=" << game.final_quaternion_scene_rotation_z()
        << " hud_boxes=" << game.hud_boxes_submitted() << " hud_images=" << game.hud_images_submitted()
        << " scene_gpu_status="
        << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
        << " scene_gpu_mesh_bindings=" << scene_gpu_stats.mesh_bindings
        << " scene_gpu_material_bindings=" << scene_gpu_stats.material_bindings
        << " scene_gpu_mesh_uploads=" << scene_gpu_stats.mesh_uploads
        << " scene_gpu_texture_uploads=" << scene_gpu_stats.texture_uploads
        << " scene_gpu_material_uploads=" << scene_gpu_stats.material_uploads
        << " scene_gpu_material_pipeline_layouts=" << scene_gpu_stats.material_pipeline_layouts
        << " scene_gpu_uploaded_texture_bytes=" << scene_gpu_stats.uploaded_texture_bytes
        << " scene_gpu_uploaded_mesh_bytes=" << scene_gpu_stats.uploaded_mesh_bytes
        << " scene_gpu_uploaded_material_factor_bytes=" << scene_gpu_stats.uploaded_material_factor_bytes
        << " scene_gpu_morph_mesh_bindings=" << scene_gpu_stats.morph_mesh_bindings
        << " scene_gpu_morph_mesh_uploads=" << scene_gpu_stats.morph_mesh_uploads
        << " scene_gpu_uploaded_morph_bytes=" << scene_gpu_stats.uploaded_morph_bytes
        << " scene_gpu_mesh_resolved=" << scene_gpu_stats.mesh_bindings_resolved
        << " scene_gpu_skinned_mesh_bindings=" << scene_gpu_stats.skinned_mesh_bindings
        << " scene_gpu_skinned_mesh_uploads=" << scene_gpu_stats.skinned_mesh_uploads
        << " scene_gpu_skinned_mesh_resolved=" << scene_gpu_stats.skinned_mesh_bindings_resolved
        << " scene_gpu_material_resolved=" << scene_gpu_stats.material_bindings_resolved << " postprocess_status="
        << mirakana::win32_desktop_presentation_postprocess_status_name(report.postprocess_status)
        << " postprocess_depth_input_requested=" << (report.postprocess_depth_input_requested ? 1 : 0)
        << " postprocess_depth_input_ready=" << (report.postprocess_depth_input_ready ? 1 : 0)
        << " postprocess_policy_status="
        << mirakana::win32_desktop_presentation_postprocess_policy_status_name(postprocess_policy.status)
        << " postprocess_policy_ready=" << (postprocess_policy.ready ? 1 : 0)
        << " postprocess_policy_diagnostics=" << postprocess_policy.diagnostics_count
        << " postprocess_policy_effects=" << postprocess_policy.effect_count
        << " postprocess_policy_postprocess_passes=" << postprocess_policy.postprocess_pass_count
        << " postprocess_policy_framegraph_passes=" << postprocess_policy.framegraph_pass_count
        << " postprocess_policy_framegraph_barrier_step_budget=" << postprocess_policy.framegraph_barrier_step_budget
        << " postprocess_policy_scene_color_required=" << (postprocess_policy.scene_color_required ? 1 : 0)
        << " postprocess_policy_scene_depth_required=" << (postprocess_policy.scene_depth_required ? 1 : 0)
        << " postprocess_policy_color_grading_effect=" << (postprocess_policy.color_grading_effect ? 1 : 0)
        << " postprocess_policy_fog_effect=" << (postprocess_policy.fog_effect ? 1 : 0)
        << " postprocess_policy_backend_shader_evidence_ready="
        << (postprocess_policy.backend_shader_evidence_ready ? 1 : 0) << " postprocess_d3d12_execution_status="
        << mirakana::win32_desktop_presentation_d3d12_postprocess_execution_status_name(
               d3d12_postprocess_execution.status)
        << " postprocess_d3d12_execution_ready=" << (d3d12_postprocess_execution.ready ? 1 : 0)
        << " postprocess_d3d12_execution_selected=" << (d3d12_postprocess_execution.d3d12_backend_selected ? 1 : 0)
        << " postprocess_d3d12_execution_shader_evidence_ready="
        << (d3d12_postprocess_execution.backend_shader_evidence_ready ? 1 : 0)
        << " postprocess_d3d12_execution_expected_passes=" << d3d12_postprocess_execution.expected_postprocess_passes
        << " postprocess_d3d12_execution_passes=" << d3d12_postprocess_execution.postprocess_passes_executed
        << " postprocess_d3d12_execution_passes_ok=" << (d3d12_postprocess_execution.postprocess_passes_current ? 1 : 0)
        << " environment_fog_status="
        << mirakana::win32_desktop_presentation_environment_fog_status_name(environment_fog.status)
        << " environment_fog_ready=" << (environment_fog.ready ? 1 : 0) << " environment_fog_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_fog_requested=" << (environment_fog.requested ? 1 : 0)
        << " environment_fog_depth_input_ready=" << (environment_fog.postprocess_depth_input_ready ? 1 : 0)
        << " environment_fog_constant_buffer_ready=" << (environment_fog.constant_buffer_ready ? 1 : 0)
        << " environment_fog_constants_binding=" << environment_fog.constants_binding
        << " environment_fog_constants_byte_size=" << environment_fog.constant_buffer_bytes
        << " environment_fog_postprocess_passes_ok=" << (environment_fog.postprocess_passes_current ? 1 : 0)
        << " environment_fog_diagnostics=" << environment_fog.diagnostics_count
        << " environment_fog_vulkan_package_status="
        << mirakana::win32_desktop_presentation_vulkan_environment_fog_package_status_name(
               environment_fog_vulkan_package.status)
        << " environment_fog_vulkan_package_ready=" << (environment_fog_vulkan_package.ready ? 1 : 0)
        << " environment_fog_vulkan_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_fog_vulkan_requested=" << (environment_fog_vulkan_package.requested ? 1 : 0)
        << " environment_fog_vulkan_depth_input_ready="
        << (environment_fog_vulkan_package.postprocess_depth_input_ready ? 1 : 0)
        << " environment_fog_vulkan_shader_contract_evidence_ready="
        << (environment_fog_vulkan_package.shader_contract_evidence_ready ? 1 : 0)
        << " environment_fog_vulkan_package_evidence_ready="
        << (environment_fog_vulkan_package.package_evidence_ready ? 1 : 0)
        << " environment_fog_vulkan_constant_buffer_ready="
        << (environment_fog_vulkan_package.constant_buffer_ready ? 1 : 0)
        << " environment_fog_vulkan_constants_binding=" << environment_fog_vulkan_package.constants_binding
        << " environment_fog_vulkan_constants_byte_size=" << environment_fog_vulkan_package.constant_buffer_bytes
        << " environment_fog_vulkan_postprocess_passes_ok="
        << (environment_fog_vulkan_package.postprocess_passes_current ? 1 : 0)
        << " environment_fog_vulkan_native_handle_access="
        << (environment_fog_vulkan_package.exposes_native_handles ? 1 : 0)
        << " environment_fog_vulkan_diagnostics=" << environment_fog_vulkan_package.diagnostics_count
        << " environment_physical_sky_status="
        << mirakana::win32_desktop_presentation_physical_sky_status_name(physical_sky.status)
        << " environment_physical_sky_ready=" << (physical_sky.ready ? 1 : 0)
        << " environment_physical_sky_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_physical_sky_requested=" << (physical_sky.requested ? 1 : 0)
        << " environment_physical_sky_shader_contract_evidence_ready="
        << (physical_sky.shader_contract_evidence_ready ? 1 : 0)
        << " environment_physical_sky_package_evidence_ready=" << (physical_sky.package_evidence_ready ? 1 : 0)
        << " environment_physical_sky_execution_evidence_ready=" << (physical_sky.execution_evidence_ready ? 1 : 0)
        << " environment_physical_sky_constant_buffer_ready=" << (physical_sky.constant_buffer_ready ? 1 : 0)
        << " environment_physical_sky_constants_binding=" << physical_sky.constants_binding
        << " environment_physical_sky_constants_byte_size=" << physical_sky.constant_buffer_bytes
        << " environment_physical_sky_constant_layout_rows=" << physical_sky.constant_layout_rows
        << " environment_physical_sky_lut_intent_rows=" << physical_sky.lut_intent_rows
        << " environment_physical_sky_lut_texture_allocations=" << (physical_sky.allocates_lut_textures ? 1 : 0)
        << " environment_physical_sky_backend_invocations=" << (physical_sky.invokes_backend ? 1 : 0)
        << " environment_physical_sky_native_handle_access=" << (physical_sky.exposes_native_handles ? 1 : 0)
        << " environment_physical_sky_diagnostics=" << physical_sky.diagnostics_count
        << " environment_physical_sky_vulkan_status="
        << mirakana::win32_desktop_presentation_vulkan_physical_sky_package_status_name(
               physical_sky_vulkan_package.status)
        << " environment_physical_sky_vulkan_ready=" << (physical_sky_vulkan_package.ready ? 1 : 0)
        << " environment_physical_sky_vulkan_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_physical_sky_vulkan_requested=" << (physical_sky_vulkan_package.requested ? 1 : 0)
        << " environment_physical_sky_vulkan_shader_contract_evidence_ready="
        << (physical_sky_vulkan_package.shader_contract_evidence_ready ? 1 : 0)
        << " environment_physical_sky_vulkan_package_evidence_ready="
        << (physical_sky_vulkan_package.package_evidence_ready ? 1 : 0)
        << " environment_physical_sky_vulkan_execution_evidence_ready="
        << (physical_sky_vulkan_package.execution_evidence_ready ? 1 : 0)
        << " environment_physical_sky_vulkan_constant_buffer_ready="
        << (physical_sky_vulkan_package.constant_buffer_ready ? 1 : 0)
        << " environment_physical_sky_vulkan_constants_binding=" << physical_sky_vulkan_package.constants_binding
        << " environment_physical_sky_vulkan_constants_byte_size=" << physical_sky_vulkan_package.constant_buffer_bytes
        << " environment_physical_sky_vulkan_constant_layout_rows=" << physical_sky_vulkan_package.constant_layout_rows
        << " environment_physical_sky_vulkan_lut_intent_rows=" << physical_sky_vulkan_package.lut_intent_rows
        << " environment_physical_sky_vulkan_lut_texture_allocations="
        << (physical_sky_vulkan_package.allocates_lut_textures ? 1 : 0)
        << " environment_physical_sky_vulkan_backend_invocations="
        << (physical_sky_vulkan_package.invokes_backend ? 1 : 0)
        << " environment_physical_sky_vulkan_native_handle_access="
        << (physical_sky_vulkan_package.exposes_native_handles ? 1 : 0)
        << " environment_physical_sky_vulkan_diagnostics=" << physical_sky_vulkan_package.diagnostics_count
        << " environment_lighting_status=" << environment_lighting_package_status_name(environment_lighting.status)
        << " environment_lighting_ready=" << (environment_lighting.ready ? 1 : 0)
        << " environment_lighting_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_lighting_requested=" << (environment_lighting.requested ? 1 : 0)
        << " environment_lighting_dependency_mode=first_party_cooked_texture"
        << " environment_lighting_visual_sky_source=physical_sky"
        << " environment_lighting_lighting_sky_source=specified_cubemap"
        << " environment_lighting_package_file=" << (environment_lighting.package_file_present ? 1 : 0)
        << " environment_lighting_package_index_entry=" << (environment_lighting.package_index_entry_present ? 1 : 0)
        << " environment_lighting_package_record_ready=" << (environment_lighting.package_record_ready ? 1 : 0)
        << " environment_lighting_package_source_ready=" << (environment_lighting.package_source_ready ? 1 : 0)
        << " environment_lighting_hdr_metadata_ready=" << (environment_lighting.hdr_metadata_ready ? 1 : 0)
        << " environment_lighting_package_evidence_ready=" << (environment_lighting.package_evidence_ready ? 1 : 0)
        << " environment_lighting_reflection_cubemap_rows=" << environment_lighting.reflection_cubemap_rows
        << " environment_lighting_reflection_cubemap_face_count=" << environment_lighting.reflection_cubemap_face_count
        << " environment_lighting_reflection_cubemap_edge_size=" << environment_lighting.reflection_cubemap_edge_size
        << " environment_lighting_reflection_cubemap_mip_count=" << environment_lighting.reflection_cubemap_mip_count
        << " environment_lighting_reflection_cubemap_format="
        << environment_lighting_texture_format_name(environment_lighting.reflection_cubemap_format)
        << " environment_lighting_irradiance_rows=" << environment_lighting.irradiance_rows
        << " environment_lighting_radiance_mip_rows=" << environment_lighting.radiance_mip_rows
        << " environment_lighting_package_evidence_rows=" << environment_lighting.package_evidence_rows
        << " environment_lighting_hdr_clamp_enabled=" << (environment_lighting.hdr_clamp_enabled ? 1 : 0)
        << " environment_lighting_hdr_clamp_max_luminance_nits=" << environment_lighting.hdr_clamp_max_luminance_nits
        << " environment_lighting_renderer_upload_status="
        << environment_lighting_renderer_upload_status_name(environment_lighting)
        << " environment_lighting_renderer_execution_status="
        << mirakana::win32_desktop_presentation_environment_ibl_renderer_execution_status_name(
               environment_ibl_renderer_execution.status)
        << " environment_lighting_renderer_upload_evidence_ready="
        << (environment_lighting.renderer_upload_evidence_ready ? 1 : 0)
        << " environment_lighting_runtime_capture_evidence_ready="
        << (environment_lighting.runtime_capture_evidence_ready ? 1 : 0)
        << " environment_lighting_texture_uploads=" << environment_lighting.texture_uploads
        << " environment_lighting_texture_cube_uploads=" << environment_lighting.texture_cube_uploads
        << " environment_lighting_texture_cube_faces=" << environment_lighting.texture_cube_faces
        << " environment_lighting_texture_cube_edge_size=" << environment_lighting.texture_cube_edge_size
        << " environment_lighting_radiance_mips=" << environment_lighting.renderer_radiance_mips
        << " environment_lighting_renderer_irradiance_rows=" << environment_lighting.renderer_irradiance_rows
        << " environment_lighting_shader_sampling_proven=" << (environment_lighting.shader_sampling_proven ? 1 : 0)
        << " environment_lighting_shader_sample_readback_nonzero="
        << (environment_lighting.shader_sample_readback_nonzero ? 1 : 0)
        << " environment_lighting_backend_invocations=" << environment_lighting.backend_invocations
        << " environment_lighting_runtime_captures=" << environment_lighting.runtime_captures
        << " environment_lighting_runtime_capture_faces=" << environment_lighting.runtime_capture_faces
        << " environment_lighting_runtime_capture_readback_nonzero="
        << (environment_lighting.runtime_capture_readback_nonzero ? 1 : 0)
        << " environment_lighting_runtime_capture_readback_checksum="
        << environment_lighting.runtime_capture_readback_checksum
        << " environment_lighting_native_handle_access=" << (environment_lighting.exposes_native_handles ? 1 : 0)
        << " environment_lighting_diagnostics=" << environment_lighting.diagnostics
        << " environment_lighting_vulkan_status="
        << mirakana::win32_desktop_presentation_vulkan_environment_ibl_renderer_execution_status_name(
               environment_ibl_vulkan_renderer_execution.status)
        << " environment_lighting_vulkan_ready=" << (environment_ibl_vulkan_renderer_execution.ready ? 1 : 0)
        << " environment_lighting_vulkan_selected_backend="
        << (environment_ibl_vulkan_renderer_execution.vulkan_backend_selected ? "vulkan" : "not_vulkan")
        << " environment_lighting_vulkan_requested=" << (environment_ibl_vulkan_renderer_execution.requested ? 1 : 0)
        << " environment_lighting_vulkan_renderer_upload_status="
        << mirakana::win32_desktop_presentation_vulkan_environment_ibl_renderer_execution_status_name(
               environment_ibl_vulkan_renderer_execution.status)
        << " environment_lighting_vulkan_renderer_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_environment_ibl_renderer_execution_status_name(
               environment_ibl_vulkan_renderer_execution.status)
        << " environment_lighting_vulkan_texture_cube_uploads="
        << environment_ibl_vulkan_renderer_execution.texture_cube_uploads
        << " environment_lighting_vulkan_texture_cube_faces="
        << environment_ibl_vulkan_renderer_execution.texture_cube_faces
        << " environment_lighting_vulkan_texture_cube_edge_size="
        << environment_ibl_vulkan_renderer_execution.texture_cube_edge_size
        << " environment_lighting_vulkan_radiance_mips=" << environment_ibl_vulkan_renderer_execution.radiance_mips
        << " environment_lighting_vulkan_renderer_irradiance_rows="
        << environment_ibl_vulkan_renderer_execution.irradiance_rows
        << " environment_lighting_vulkan_cube_compatible_image_created="
        << (environment_ibl_vulkan_renderer_execution.cube_compatible_image_created ? 1 : 0)
        << " environment_lighting_vulkan_cube_image_view_created="
        << (environment_ibl_vulkan_renderer_execution.cube_image_view_created ? 1 : 0)
        << " environment_lighting_vulkan_sampler_created="
        << (environment_ibl_vulkan_renderer_execution.sampler_created ? 1 : 0)
        << " environment_lighting_vulkan_descriptor_set_bindings="
        << environment_ibl_vulkan_renderer_execution.descriptor_set_bindings
        << " environment_lighting_vulkan_synchronization2_barriers="
        << environment_ibl_vulkan_renderer_execution.synchronization2_barriers
        << " environment_lighting_vulkan_shader_sampling_proven="
        << (environment_ibl_vulkan_renderer_execution.shader_sampling_proven ? 1 : 0)
        << " environment_lighting_vulkan_shader_sample_readback_nonzero="
        << (environment_ibl_vulkan_renderer_execution.shader_sample_readback_nonzero ? 1 : 0)
        << " environment_lighting_vulkan_backend_invocations="
        << (environment_ibl_vulkan_renderer_execution.texture_cube_uploads > 0U ? 1 : 0)
        << " environment_lighting_vulkan_runtime_captures="
        << (environment_ibl_vulkan_renderer_execution.runtime_capture_faces > 0U ? 1 : 0)
        << " environment_lighting_vulkan_runtime_capture_faces="
        << environment_ibl_vulkan_renderer_execution.runtime_capture_faces
        << " environment_lighting_vulkan_runtime_capture_readback_nonzero="
        << (environment_ibl_vulkan_renderer_execution.runtime_capture_readback_nonzero ? 1 : 0)
        << " environment_lighting_vulkan_runtime_capture_readback_checksum="
        << environment_ibl_vulkan_renderer_execution.runtime_capture_readback_checksum
        << " environment_lighting_vulkan_native_handle_access="
        << environment_ibl_vulkan_renderer_execution.native_handle_access
        << " environment_lighting_vulkan_diagnostics=" << environment_ibl_vulkan_renderer_execution.diagnostics_count
        << " cloud_layer_status=" << mirakana::win32_desktop_presentation_cloud_layer_status_name(cloud_layer.status)
        << " cloud_layer_ready=" << (cloud_layer.ready ? 1 : 0) << " cloud_layer_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " cloud_layer_requested=" << (cloud_layer.requested ? 1 : 0)
        << " cloud_layer_shader_contract_evidence_ready=" << (cloud_layer.shader_contract_evidence_ready ? 1 : 0)
        << " cloud_layer_package_evidence_ready=" << (cloud_layer.package_evidence_ready ? 1 : 0)
        << " cloud_layer_execution_evidence_ready=" << (cloud_layer.execution_evidence_ready ? 1 : 0)
        << " cloud_layer_cloud_map_binding=" << cloud_layer.cloud_map_binding
        << " cloud_layer_flow_map_binding=" << cloud_layer.flow_map_binding
        << " cloud_layer_sampler_binding=" << cloud_layer.sampler_binding
        << " cloud_layer_constants_binding=" << cloud_layer.constants_binding
        << " cloud_layer_uses_latlong_projection=" << (cloud_layer.uses_latlong_projection ? 1 : 0)
        << " cloud_layer_uses_flow_map=" << (cloud_layer.uses_flow_map ? 1 : 0)
        << " cloud_layer_texture_rows=" << cloud_layer.texture_rows
        << " cloud_layer_visual_rows=" << cloud_layer.visual_rows << " cloud_layer_ibl_rows=" << cloud_layer.ibl_rows
        << " cloud_layer_shader_contract_rows=" << cloud_layer.shader_contract_rows
        << " cloud_layer_quality_rows=" << cloud_layer.quality_rows
        << " cloud_layer_texture_uploads=" << (cloud_layer.uploads_textures ? 1 : 0)
        << " cloud_layer_backend_invocations=" << (cloud_layer.invokes_backend ? 1 : 0)
        << " cloud_layer_renderer_draws=" << cloud_layer.renderer_draws
        << " cloud_layer_native_handle_access=" << (cloud_layer.exposes_native_handles ? 1 : 0)
        << " cloud_layer_volumetric_clouds=" << (cloud_layer.uses_volumetric_clouds ? 1 : 0)
        << " cloud_layer_diagnostics=" << cloud_layer.diagnostics_count << " environment_precipitation_status="
        << mirakana::win32_desktop_presentation_environment_precipitation_status_name(environment_precipitation.status)
        << " environment_precipitation_ready=" << (environment_precipitation.ready ? 1 : 0)
        << " environment_precipitation_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_precipitation_requested=" << (environment_precipitation.requested ? 1 : 0)
        << " environment_precipitation_weather="
        << mirakana::environment_weather_kind_name(environment_precipitation.weather)
        << " environment_precipitation_kind="
        << mirakana::environment_precipitation_kind_name(environment_precipitation.kind)
        << " environment_precipitation_shader_contract_evidence_ready="
        << (environment_precipitation.shader_contract_evidence_ready ? 1 : 0)
        << " environment_precipitation_package_evidence_ready="
        << (environment_precipitation.package_evidence_ready ? 1 : 0)
        << " environment_precipitation_execution_evidence_ready="
        << (environment_precipitation.execution_evidence_ready ? 1 : 0)
        << " environment_precipitation_particle_texture_binding=" << environment_precipitation.particle_texture_binding
        << " environment_precipitation_scene_depth_texture_binding="
        << environment_precipitation.scene_depth_texture_binding
        << " environment_precipitation_sampler_binding=" << environment_precipitation.sampler_binding
        << " environment_precipitation_constants_binding=" << environment_precipitation.constants_binding
        << " environment_precipitation_uses_camera_near_particles="
        << (environment_precipitation.uses_camera_near_particles ? 1 : 0)
        << " environment_precipitation_uses_scene_depth_occlusion="
        << (environment_precipitation.uses_scene_depth_occlusion ? 1 : 0)
        << " environment_precipitation_weather_rows=" << environment_precipitation.weather_rows
        << " environment_precipitation_particle_rows=" << environment_precipitation.particle_rows
        << " environment_precipitation_occlusion_rows=" << environment_precipitation.occlusion_rows
        << " environment_precipitation_wetness_rows=" << environment_precipitation.wetness_rows
        << " environment_precipitation_audio_handoff_rows=" << environment_precipitation.audio_handoff_rows
        << " environment_precipitation_shader_rows=" << environment_precipitation.shader_rows
        << " environment_precipitation_quality_rows=" << environment_precipitation.quality_rows
        << " environment_precipitation_particle_buffer_uploads="
        << (environment_precipitation.uploads_particle_buffers ? 1 : 0)
        << " environment_precipitation_backend_invocations=" << (environment_precipitation.invokes_backend ? 1 : 0)
        << " environment_precipitation_renderer_draws=" << environment_precipitation.renderer_draws
        << " environment_precipitation_depth_occlusion_readback="
        << (environment_precipitation.depth_occlusion_readback ? 1 : 0)
        << " environment_precipitation_native_handle_access="
        << (environment_precipitation.exposes_native_handles ? 1 : 0)
        << " environment_precipitation_material_mutations=" << (environment_precipitation.mutates_materials ? 1 : 0)
        << " environment_precipitation_audio_playback=" << (environment_precipitation.plays_audio ? 1 : 0)
        << " environment_precipitation_diagnostics=" << environment_precipitation.diagnostics_count
        << " environment_precipitation_vulkan_status="
        << mirakana::win32_desktop_presentation_vulkan_environment_precipitation_status_name(
               environment_precipitation_vulkan.status)
        << " environment_precipitation_vulkan_ready=" << (environment_precipitation_vulkan.ready ? 1 : 0)
        << " environment_precipitation_vulkan_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_precipitation_vulkan_requested=" << (environment_precipitation_vulkan.requested ? 1 : 0)
        << " environment_precipitation_vulkan_weather="
        << mirakana::environment_weather_kind_name(environment_precipitation_vulkan.weather)
        << " environment_precipitation_vulkan_kind="
        << mirakana::environment_precipitation_kind_name(environment_precipitation_vulkan.kind)
        << " environment_precipitation_vulkan_shader_contract_evidence_ready="
        << (environment_precipitation_vulkan.shader_contract_evidence_ready ? 1 : 0)
        << " environment_precipitation_vulkan_package_evidence_ready="
        << (environment_precipitation_vulkan.package_evidence_ready ? 1 : 0)
        << " environment_precipitation_vulkan_execution_evidence_ready="
        << (environment_precipitation_vulkan.execution_evidence_ready ? 1 : 0)
        << " environment_precipitation_vulkan_particle_texture_binding="
        << environment_precipitation_vulkan.particle_texture_binding
        << " environment_precipitation_vulkan_scene_depth_texture_binding="
        << environment_precipitation_vulkan.scene_depth_texture_binding
        << " environment_precipitation_vulkan_sampler_binding=" << environment_precipitation_vulkan.sampler_binding
        << " environment_precipitation_vulkan_constants_binding=" << environment_precipitation_vulkan.constants_binding
        << " environment_precipitation_vulkan_uses_camera_near_particles="
        << (environment_precipitation_vulkan.uses_camera_near_particles ? 1 : 0)
        << " environment_precipitation_vulkan_uses_scene_depth_occlusion="
        << (environment_precipitation_vulkan.uses_scene_depth_occlusion ? 1 : 0)
        << " environment_precipitation_vulkan_weather_rows=" << environment_precipitation_vulkan.weather_rows
        << " environment_precipitation_vulkan_particle_rows=" << environment_precipitation_vulkan.particle_rows
        << " environment_precipitation_vulkan_occlusion_rows=" << environment_precipitation_vulkan.occlusion_rows
        << " environment_precipitation_vulkan_wetness_rows=" << environment_precipitation_vulkan.wetness_rows
        << " environment_precipitation_vulkan_audio_handoff_rows="
        << environment_precipitation_vulkan.audio_handoff_rows
        << " environment_precipitation_vulkan_shader_rows=" << environment_precipitation_vulkan.shader_rows
        << " environment_precipitation_vulkan_quality_rows=" << environment_precipitation_vulkan.quality_rows
        << " environment_precipitation_vulkan_particle_buffer_uploads="
        << (environment_precipitation_vulkan.uploads_particle_buffers ? 1 : 0)
        << " environment_precipitation_vulkan_backend_invocations="
        << (environment_precipitation_vulkan.invokes_backend ? 1 : 0)
        << " environment_precipitation_vulkan_renderer_draws=" << environment_precipitation_vulkan.renderer_draws
        << " environment_precipitation_vulkan_depth_occlusion_readback="
        << (environment_precipitation_vulkan.depth_occlusion_readback ? 1 : 0)
        << " environment_precipitation_vulkan_descriptor_set_bindings="
        << environment_precipitation_vulkan.descriptor_set_bindings
        << " environment_precipitation_vulkan_synchronization2_barriers="
        << environment_precipitation_vulkan.synchronization2_barriers
        << " environment_precipitation_vulkan_native_handle_access="
        << (environment_precipitation_vulkan.exposes_native_handles ? 1 : 0)
        << " environment_precipitation_vulkan_material_mutations="
        << (environment_precipitation_vulkan.mutates_materials ? 1 : 0)
        << " environment_precipitation_vulkan_audio_playback=" << (environment_precipitation_vulkan.plays_audio ? 1 : 0)
        << " environment_precipitation_vulkan_diagnostics=" << environment_precipitation_vulkan.diagnostics_count
        << " environment_audio_playback_status="
        << environment_audio_playback_status_name(environment_audio_playback.status)
        << " environment_audio_playback_ready=" << (environment_audio_playback.ready ? 1 : 0)
        << " environment_audio_playback_requested=" << (environment_audio_playback.requested ? 1 : 0)
        << " environment_audio_runtime_lane_ready=" << (environment_audio_playback.runtime_audio_lane_ready ? 1 : 0)
        << " environment_audio_trigger_rows=" << environment_audio_playback.trigger_rows
        << " environment_audio_runtime_cues_started=" << environment_audio_playback.runtime_cues_started
        << " environment_audio_runtime_cues_stopped=" << environment_audio_playback.runtime_cues_stopped
        << " environment_audio_runtime_render_commands=" << environment_audio_playback.runtime_render_commands
        << " environment_audio_runtime_render_frames=" << environment_audio_playback.runtime_render_frames
        << " environment_audio_device_host_evidence="
        << (environment_audio_playback.device_host_evidence_available ? 1 : 0)
        << " environment_audio_device_io_invoked=" << (environment_audio_playback.device_io_invoked ? 1 : 0)
        << " environment_audio_device_owned_by_environment="
        << (environment_audio_playback.device_owned_by_environment ? 1 : 0)
        << " environment_audio_native_handle_access=" << (environment_audio_playback.native_handle_access ? 1 : 0)
        << " environment_audio_diagnostics=" << environment_audio_playback.diagnostics
        << " environment_material_weathering_status="
        << environment_material_weathering_status_name(environment_material_weathering.status)
        << " environment_material_weathering_ready=" << (environment_material_weathering.ready ? 1 : 0)
        << " environment_material_weathering_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_material_weathering_requested=" << (environment_material_weathering.requested ? 1 : 0)
        << " environment_material_weathering_state="
        << environment_material_weathering_state_name(environment_material_weathering.state)
        << " environment_material_weathering_shader_contract_evidence_ready="
        << (environment_material_weathering.shader_contract_evidence_ready ? 1 : 0)
        << " environment_material_weathering_package_evidence_ready="
        << (environment_material_weathering.package_evidence_ready ? 1 : 0)
        << " environment_material_weathering_execution_evidence_ready="
        << (environment_material_weathering.execution_evidence_ready ? 1 : 0)
        << " environment_material_weathering_constants_binding=" << environment_material_weathering.constants_binding
        << " environment_material_weathering_constants_byte_size="
        << environment_material_weathering.constant_layout_bytes
        << " environment_material_weathering_constant_rows=" << environment_material_weathering.constant_rows
        << " environment_material_weathering_wet_rows=" << environment_material_weathering.wet_rows
        << " environment_material_weathering_snow_rows=" << environment_material_weathering.snow_rows
        << " environment_material_weathering_ice_rows=" << environment_material_weathering.ice_rows
        << " environment_material_weathering_quality_rows=" << environment_material_weathering.quality_rows
        << " environment_material_weathering_material_parameter_bindings="
        << environment_material_weathering.material_parameter_bindings
        << " environment_material_weathering_material_constant_bytes="
        << environment_material_weathering.material_constant_bytes
        << " environment_material_weathering_backend_invocations="
        << environment_material_weathering.backend_invocations
        << " environment_material_weathering_source_material_mutations="
        << (environment_material_weathering.source_material_mutations ? 1 : 0)
        << " environment_material_weathering_native_handle_access="
        << (environment_material_weathering.native_handle_access ? 1 : 0)
        << " environment_material_weathering_diagnostics=" << environment_material_weathering.diagnostics
        << " environment_volumetric_fog_status="
        << mirakana::win32_desktop_presentation_environment_volumetric_fog_status_name(
               environment_volumetric_fog.status)
        << " environment_volumetric_fog_ready=" << (environment_volumetric_fog.ready ? 1 : 0)
        << " environment_volumetric_fog_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_volumetric_fog_requested=" << (environment_volumetric_fog.requested ? 1 : 0)
        << " environment_volumetric_fog_shader_contract_evidence_ready="
        << (environment_volumetric_fog.shader_contract_evidence_ready ? 1 : 0)
        << " environment_volumetric_fog_package_evidence_ready="
        << (environment_volumetric_fog.package_evidence_ready ? 1 : 0)
        << " environment_volumetric_fog_execution_evidence_ready="
        << (environment_volumetric_fog.execution_evidence_ready ? 1 : 0)
        << " environment_volumetric_fog_froxel_output_ready="
        << (environment_volumetric_fog.froxel_output_ready ? 1 : 0)
        << " environment_volumetric_fog_scene_depth_ready=" << (environment_volumetric_fog.scene_depth_ready ? 1 : 0)
        << " environment_volumetric_fog_compute_dispatches=" << environment_volumetric_fog.compute_dispatches
        << " environment_volumetric_fog_constants_binding=" << environment_volumetric_fog.constants_binding
        << " environment_volumetric_fog_constants_byte_size=" << environment_volumetric_fog.constant_buffer_bytes
        << " environment_volumetric_fog_froxel_output_binding="
        << environment_volumetric_fog.froxel_output_buffer_binding
        << " environment_volumetric_fog_native_handle_access="
        << (environment_volumetric_fog.exposes_native_handles ? 1 : 0)
        << " environment_volumetric_fog_diagnostics=" << environment_volumetric_fog.diagnostics_count
        << " environment_volumetric_fog_vulkan_status="
        << mirakana::win32_desktop_presentation_vulkan_environment_volumetric_fog_status_name(
               environment_volumetric_fog_vulkan.status)
        << " environment_volumetric_fog_vulkan_ready=" << (environment_volumetric_fog_vulkan.ready ? 1 : 0)
        << " environment_volumetric_fog_vulkan_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_volumetric_fog_vulkan_requested=" << (environment_volumetric_fog_vulkan.requested ? 1 : 0)
        << " environment_volumetric_fog_vulkan_shader_contract_evidence_ready="
        << (environment_volumetric_fog_vulkan.shader_contract_evidence_ready ? 1 : 0)
        << " environment_volumetric_fog_vulkan_package_evidence_ready="
        << (environment_volumetric_fog_vulkan.package_evidence_ready ? 1 : 0)
        << " environment_volumetric_fog_vulkan_execution_evidence_ready="
        << (environment_volumetric_fog_vulkan.execution_evidence_ready ? 1 : 0)
        << " environment_volumetric_fog_vulkan_froxel_output_ready="
        << (environment_volumetric_fog_vulkan.froxel_output_ready ? 1 : 0)
        << " environment_volumetric_fog_vulkan_scene_depth_ready="
        << (environment_volumetric_fog_vulkan.scene_depth_ready ? 1 : 0)
        << " environment_volumetric_fog_vulkan_compute_dispatches="
        << environment_volumetric_fog_vulkan.compute_dispatches
        << " environment_volumetric_fog_vulkan_descriptor_set_bindings="
        << environment_volumetric_fog_vulkan.descriptor_set_bindings
        << " environment_volumetric_fog_vulkan_synchronization2_barriers="
        << environment_volumetric_fog_vulkan.synchronization2_barriers
        << " environment_volumetric_fog_vulkan_froxel_readback_nonzero="
        << (environment_volumetric_fog_vulkan.froxel_readback_nonzero ? 1 : 0)
        << " environment_volumetric_fog_vulkan_scene_depth_texture_binding="
        << environment_volumetric_fog_vulkan.scene_depth_texture_binding
        << " environment_volumetric_fog_vulkan_scene_depth_sampler_binding="
        << environment_volumetric_fog_vulkan.scene_depth_sampler_binding
        << " environment_volumetric_fog_vulkan_constants_binding="
        << environment_volumetric_fog_vulkan.constants_binding
        << " environment_volumetric_fog_vulkan_constants_byte_size="
        << environment_volumetric_fog_vulkan.constant_buffer_bytes
        << " environment_volumetric_fog_vulkan_froxel_output_storage_buffer_binding="
        << environment_volumetric_fog_vulkan.froxel_output_buffer_binding
        << " environment_volumetric_fog_vulkan_native_handle_access="
        << (environment_volumetric_fog_vulkan.exposes_native_handles ? 1 : 0)
        << " environment_volumetric_fog_vulkan_diagnostics=" << environment_volumetric_fog_vulkan.diagnostics_count
        << " environment_volumetric_cloud_status="
        << mirakana::win32_desktop_presentation_environment_volumetric_cloud_status_name(
               environment_volumetric_cloud.status)
        << " environment_volumetric_cloud_ready=" << (environment_volumetric_cloud.ready ? 1 : 0)
        << " environment_volumetric_cloud_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_volumetric_cloud_requested=" << (environment_volumetric_cloud.requested ? 1 : 0)
        << " environment_volumetric_cloud_shader_contract_evidence_ready="
        << (environment_volumetric_cloud.shader_contract_evidence_ready ? 1 : 0)
        << " environment_volumetric_cloud_package_evidence_ready="
        << (environment_volumetric_cloud.package_evidence_ready ? 1 : 0)
        << " environment_volumetric_cloud_execution_evidence_ready="
        << (environment_volumetric_cloud.execution_evidence_ready ? 1 : 0)
        << " environment_volumetric_cloud_weather_map_binding=" << environment_volumetric_cloud.weather_map_binding
        << " environment_volumetric_cloud_shape_noise_binding=" << environment_volumetric_cloud.shape_noise_binding
        << " environment_volumetric_cloud_erosion_noise_binding=" << environment_volumetric_cloud.erosion_noise_binding
        << " environment_volumetric_cloud_sampler_binding=" << environment_volumetric_cloud.sampler_binding
        << " environment_volumetric_cloud_constants_binding=" << environment_volumetric_cloud.constants_binding
        << " environment_volumetric_cloud_constants_byte_size=" << environment_volumetric_cloud.constant_buffer_bytes
        << " environment_volumetric_cloud_weather_map_ready="
        << (environment_volumetric_cloud.weather_map_ready ? 1 : 0)
        << " environment_volumetric_cloud_shape_noise_ready="
        << (environment_volumetric_cloud.shape_noise_ready ? 1 : 0)
        << " environment_volumetric_cloud_erosion_noise_ready="
        << (environment_volumetric_cloud.erosion_noise_ready ? 1 : 0)
        << " environment_volumetric_cloud_map_rows=" << environment_volumetric_cloud.map_rows
        << " environment_volumetric_cloud_layer_rows=" << environment_volumetric_cloud.layer_rows
        << " environment_volumetric_cloud_lighting_rows=" << environment_volumetric_cloud.lighting_rows
        << " environment_volumetric_cloud_raymarch_rows=" << environment_volumetric_cloud.raymarch_rows
        << " environment_volumetric_cloud_temporal_rows=" << environment_volumetric_cloud.temporal_rows
        << " environment_volumetric_cloud_shadow_rows=" << environment_volumetric_cloud.shadow_rows
        << " environment_volumetric_cloud_storm_rows=" << environment_volumetric_cloud.storm_rows
        << " environment_volumetric_cloud_shader_contract_rows=" << environment_volumetric_cloud.shader_contract_rows
        << " environment_volumetric_cloud_quality_rows=" << environment_volumetric_cloud.quality_rows
        << " environment_volumetric_cloud_texture_uploads="
        << (environment_volumetric_cloud.uploads_volume_textures ? 1 : 0)
        << " environment_volumetric_cloud_backend_invocations="
        << (environment_volumetric_cloud.invokes_backend ? 1 : 0)
        << " environment_volumetric_cloud_renderer_draws=" << environment_volumetric_cloud.renderer_draws
        << " environment_volumetric_cloud_raymarch_passes=" << environment_volumetric_cloud.raymarch_passes
        << " environment_volumetric_cloud_readback_nonzero=" << (environment_volumetric_cloud.readback_nonzero ? 1 : 0)
        << " environment_volumetric_cloud_native_handle_access="
        << (environment_volumetric_cloud.exposes_native_handles ? 1 : 0)
        << " environment_volumetric_cloud_audio_playback=" << (environment_volumetric_cloud.plays_audio ? 1 : 0)
        << " environment_volumetric_cloud_precipitation_execution="
        << (environment_volumetric_cloud.executes_precipitation ? 1 : 0)
        << " environment_volumetric_cloud_diagnostics=" << environment_volumetric_cloud.diagnostics_count
        << " environment_volumetric_cloud_vulkan_status="
        << mirakana::win32_desktop_presentation_vulkan_environment_volumetric_cloud_status_name(
               environment_volumetric_cloud_vulkan.status)
        << " environment_volumetric_cloud_vulkan_ready=" << (environment_volumetric_cloud_vulkan.ready ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_selected_backend="
        << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " environment_volumetric_cloud_vulkan_requested=" << (environment_volumetric_cloud_vulkan.requested ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_shader_contract_evidence_ready="
        << (environment_volumetric_cloud_vulkan.shader_contract_evidence_ready ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_package_evidence_ready="
        << (environment_volumetric_cloud_vulkan.package_evidence_ready ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_execution_evidence_ready="
        << (environment_volumetric_cloud_vulkan.execution_evidence_ready ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_weather_map_binding="
        << environment_volumetric_cloud_vulkan.weather_map_binding
        << " environment_volumetric_cloud_vulkan_shape_noise_binding="
        << environment_volumetric_cloud_vulkan.shape_noise_binding
        << " environment_volumetric_cloud_vulkan_erosion_noise_binding="
        << environment_volumetric_cloud_vulkan.erosion_noise_binding
        << " environment_volumetric_cloud_vulkan_sampler_binding="
        << environment_volumetric_cloud_vulkan.sampler_binding
        << " environment_volumetric_cloud_vulkan_constants_binding="
        << environment_volumetric_cloud_vulkan.constants_binding
        << " environment_volumetric_cloud_vulkan_constants_byte_size="
        << environment_volumetric_cloud_vulkan.constant_buffer_bytes
        << " environment_volumetric_cloud_vulkan_weather_map_ready="
        << (environment_volumetric_cloud_vulkan.weather_map_ready ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_shape_noise_ready="
        << (environment_volumetric_cloud_vulkan.shape_noise_ready ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_erosion_noise_ready="
        << (environment_volumetric_cloud_vulkan.erosion_noise_ready ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_map_rows=" << environment_volumetric_cloud_vulkan.map_rows
        << " environment_volumetric_cloud_vulkan_layer_rows=" << environment_volumetric_cloud_vulkan.layer_rows
        << " environment_volumetric_cloud_vulkan_lighting_rows=" << environment_volumetric_cloud_vulkan.lighting_rows
        << " environment_volumetric_cloud_vulkan_raymarch_rows=" << environment_volumetric_cloud_vulkan.raymarch_rows
        << " environment_volumetric_cloud_vulkan_temporal_rows=" << environment_volumetric_cloud_vulkan.temporal_rows
        << " environment_volumetric_cloud_vulkan_shadow_rows=" << environment_volumetric_cloud_vulkan.shadow_rows
        << " environment_volumetric_cloud_vulkan_storm_rows=" << environment_volumetric_cloud_vulkan.storm_rows
        << " environment_volumetric_cloud_vulkan_shader_contract_rows="
        << environment_volumetric_cloud_vulkan.shader_contract_rows
        << " environment_volumetric_cloud_vulkan_quality_rows=" << environment_volumetric_cloud_vulkan.quality_rows
        << " environment_volumetric_cloud_vulkan_descriptor_set_bindings="
        << environment_volumetric_cloud_vulkan.descriptor_set_bindings
        << " environment_volumetric_cloud_vulkan_synchronization2_barriers="
        << environment_volumetric_cloud_vulkan.synchronization2_barriers
        << " environment_volumetric_cloud_vulkan_texture_uploads="
        << (environment_volumetric_cloud_vulkan.uploads_volume_textures ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_backend_invocations="
        << (environment_volumetric_cloud_vulkan.invokes_backend ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_renderer_draws=" << environment_volumetric_cloud_vulkan.renderer_draws
        << " environment_volumetric_cloud_vulkan_raymarch_passes="
        << environment_volumetric_cloud_vulkan.raymarch_passes
        << " environment_volumetric_cloud_vulkan_readback_nonzero="
        << (environment_volumetric_cloud_vulkan.readback_nonzero ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_native_handle_access="
        << (environment_volumetric_cloud_vulkan.exposes_native_handles ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_audio_playback="
        << (environment_volumetric_cloud_vulkan.plays_audio ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_precipitation_execution="
        << (environment_volumetric_cloud_vulkan.executes_precipitation ? 1 : 0)
        << " environment_volumetric_cloud_vulkan_diagnostics=" << environment_volumetric_cloud_vulkan.diagnostics_count
        << " environment_profile_status=" << environment_profile_package_status_name(environment_profile.status)
        << " environment_profile_ready=" << (environment_profile.ready ? 1 : 0)
        << " environment_profile_requested=" << (environment_profile.requested ? 1 : 0)
        << " environment_profile_package_file=" << (environment_profile.package_file_present ? 1 : 0)
        << " environment_profile_package_index_entry=" << (environment_profile.package_index_entry_present ? 1 : 0)
        << " environment_profile_scene_reference=" << (environment_profile.scene_reference_present ? 1 : 0)
        << " environment_profile_scene_required=" << (environment_profile.scene_required ? 1 : 0)
        << " environment_profile_scene_dependency=" << (environment_profile.scene_dependency_present ? 1 : 0)
        << " environment_profile_dependency_edge=" << (environment_profile.dependency_edge_present ? 1 : 0)
        << " environment_profile_source_parsing=" << (environment_profile.runtime_source_parsing ? 1 : 0)
        << " environment_profile_diagnostics=" << environment_profile.diagnostics
        << " environment_profile_v2_status=" << environment_profile_package_status_name(environment_profile.status)
        << " environment_profile_v2_ready=" << (environment_profile.ready ? 1 : 0)
        << " environment_profile_v2_volume_rows=" << environment_profile.volume_rows
        << " environment_profile_v2_weather_keyframes=" << environment_profile.weather_keyframes
        << " environment_profile_v2_quality_preset=" << environment_profile.quality_preset
        << " environment_profile_v2_diagnostics=" << environment_profile.diagnostics
        << " environment_profile_v2_legacy_v1_accepted=" << (environment_profile.legacy_v1_accepted ? 1 : 0)
        << " environment_texture_asset_pipeline_package_status="
        << environment_texture_asset_pipeline_package_status_name(environment_texture_asset_pipeline.status)
        << " environment_texture_asset_pipeline_package_ready=" << (environment_texture_asset_pipeline.ready ? 1 : 0)
        << " environment_texture_asset_pipeline_package_requested="
        << (environment_texture_asset_pipeline.requested ? 1 : 0)
        << " environment_texture_asset_pipeline_package_index_entries="
        << environment_texture_asset_pipeline.package_index_entries
        << " environment_texture_asset_pipeline_metadata_records="
        << environment_texture_asset_pipeline.metadata_records
        << " environment_texture_asset_pipeline_metadata_only_records="
        << environment_texture_asset_pipeline.metadata_only_records
        << " environment_texture_asset_pipeline_openexr_records=" << environment_texture_asset_pipeline.openexr_records
        << " environment_texture_asset_pipeline_ktx2_basis_records="
        << environment_texture_asset_pipeline.ktx2_basis_records
        << " environment_texture_asset_pipeline_source_hash_rows="
        << environment_texture_asset_pipeline.source_hash_rows
        << " environment_texture_asset_pipeline_provenance_rows=" << environment_texture_asset_pipeline.provenance_rows
        << " environment_texture_asset_pipeline_license_rows=" << environment_texture_asset_pipeline.license_rows
        << " environment_texture_asset_pipeline_backend_policy_rows="
        << environment_texture_asset_pipeline.backend_policy_rows
        << " environment_texture_asset_pipeline_unsupported_host_diagnostics="
        << environment_texture_asset_pipeline.unsupported_host_diagnostics
        << " environment_texture_asset_pipeline_profile_dependency_refs="
        << environment_texture_asset_pipeline.environment_profile_dependency_refs
        << " environment_texture_asset_pipeline_dependency_edges="
        << environment_texture_asset_pipeline.environment_texture_dependency_edges
        << " environment_texture_asset_pipeline_pixel_decode_invoked="
        << (environment_texture_asset_pipeline.pixel_decode_invoked ? 1 : 0)
        << " environment_texture_asset_pipeline_basis_runtime_transcode_invoked="
        << (environment_texture_asset_pipeline.basis_runtime_transcode_invoked ? 1 : 0)
        << " environment_texture_asset_pipeline_gpu_upload_invoked="
        << (environment_texture_asset_pipeline.gpu_upload_invoked ? 1 : 0)
        << " environment_texture_asset_pipeline_broad_ready="
        << (environment_texture_asset_pipeline.broad_asset_pipeline_ready ? 1 : 0)
        << " environment_texture_asset_pipeline_diagnostics=" << environment_texture_asset_pipeline.diagnostics
        << " environment_preset_library_package_status="
        << environment_preset_library_package_status_name(environment_preset_library.status)
        << " environment_preset_library_package_ready=" << (environment_preset_library.ready ? 1 : 0)
        << " environment_preset_library_package_requested=" << (environment_preset_library.requested ? 1 : 0)
        << " environment_preset_library_package_index_entry="
        << (environment_preset_library.package_index_entry ? 1 : 0)
        << " environment_preset_library_package_file=" << (environment_preset_library.package_file ? 1 : 0)
        << " environment_preset_library_preset_count=" << environment_preset_library.preset_count
        << " environment_preset_library_required_preset_rows=" << environment_preset_library.required_preset_rows
        << " environment_preset_library_backend_feature_rows=" << environment_preset_library.backend_feature_rows
        << " environment_preset_library_dependency_refs=" << environment_preset_library.dependency_refs
        << " environment_preset_library_sample_consumption_evidence="
        << (environment_preset_library.sample_consumption_evidence ? 1 : 0)
        << " environment_preset_library_aaa_ready_claimed=" << (environment_preset_library.aaa_ready_claimed ? 1 : 0)
        << " environment_preset_library_diagnostics=" << environment_preset_library.diagnostics
        << " environment_quality_budget_status="
        << environment_quality_budget_status_name(environment_quality_budget.status)
        << " environment_quality_preset=" << environment_quality_budget.quality_preset
        << " environment_quality_budget_ready=" << (environment_quality_budget.ready ? 1 : 0)
        << " environment_quality_budget_requested=" << (environment_quality_budget.requested ? 1 : 0)
        << " environment_quality_budget_rows=" << environment_quality_budget.feature_rows
        << " environment_quality_budget_diagnostics=" << environment_quality_budget.diagnostics
        << " environment_quality_budget_violations=" << environment_quality_budget.violations
        << " environment_quality_budget_constant_buffer_bytes=" << environment_quality_budget.constant_buffer_bytes
        << " environment_quality_budget_physical_sky_sample_budget="
        << environment_quality_budget.physical_sky_sample_budget
        << " environment_quality_budget_height_fog_sample_step_budget="
        << environment_quality_budget.height_fog_sample_step_budget
        << " environment_quality_budget_volumetric_fog_raymarch_step_budget="
        << environment_quality_budget.volumetric_fog_raymarch_step_budget
        << " environment_quality_budget_volumetric_cloud_primary_step_budget="
        << environment_quality_budget.volumetric_cloud_primary_step_budget
        << " environment_quality_budget_volumetric_cloud_light_step_budget="
        << environment_quality_budget.volumetric_cloud_light_step_budget
        << " environment_quality_budget_particle_rows=" << environment_quality_budget.particle_rows
        << " environment_quality_budget_texture_upload_budget=" << environment_quality_budget.texture_upload_budget
        << " environment_quality_budget_particle_buffer_upload_budget="
        << environment_quality_budget.particle_buffer_upload_budget
        << " environment_quality_budget_renderer_draw_budget=" << environment_quality_budget.renderer_draw_budget
        << " environment_quality_budget_compute_dispatch_budget=" << environment_quality_budget.compute_dispatch_budget
        << " environment_quality_budget_raymarch_pass_budget=" << environment_quality_budget.raymarch_pass_budget
        << " environment_quality_budget_ibl_reflection_face_budget="
        << environment_quality_budget.ibl_reflection_face_budget
        << " environment_quality_budget_ibl_radiance_mip_budget=" << environment_quality_budget.ibl_radiance_mip_budget
        << " environment_quality_budget_transient_gpu_byte_estimate="
        << environment_quality_budget.transient_gpu_byte_estimate
        << " environment_quality_budget_transient_heap_allocations="
        << environment_quality_budget.transient_heap_allocations
        << " environment_quality_budget_transient_placed_allocations="
        << environment_quality_budget.transient_placed_allocations
        << " environment_quality_budget_transient_placed_resources_alive="
        << environment_quality_budget.transient_placed_resources_alive
        << " environment_quality_budget_framegraph_barrier_step_budget="
        << environment_quality_budget.framegraph_barrier_step_budget
        << " environment_quality_budget_framegraph_barrier_steps_executed="
        << environment_quality_budget.framegraph_barrier_steps_executed
        << " environment_quality_budget_native_handle_access="
        << (environment_quality_budget.native_handle_access ? 1 : 0)
        << " environment_quality_budget_broad_optimization_claimed="
        << (environment_quality_budget.broad_optimization_claimed ? 1 : 0)
        << " environment_quality_budget_broad_environment_ready_claimed="
        << (environment_quality_budget.broad_environment_ready_claimed ? 1 : 0)
        << " vulkan_postprocess_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_postprocess_execution_status_name(
               vulkan_postprocess_execution.status)
        << " vulkan_postprocess_execution_ready=" << (vulkan_postprocess_execution.ready ? 1 : 0)
        << " vulkan_postprocess_execution_selected=" << (vulkan_postprocess_execution.vulkan_backend_selected ? 1 : 0)
        << " vulkan_postprocess_execution_shader_evidence_ready="
        << (vulkan_postprocess_execution.backend_shader_evidence_ready ? 1 : 0)
        << " vulkan_postprocess_execution_expected_passes=" << vulkan_postprocess_execution.expected_postprocess_passes
        << " vulkan_postprocess_execution_passes=" << vulkan_postprocess_execution.postprocess_passes_executed
        << " vulkan_postprocess_execution_passes_ok="
        << (vulkan_postprocess_execution.postprocess_passes_current ? 1 : 0)
        << " d3d12_instanced_draw_execution_status="
        << mirakana::win32_desktop_presentation_d3d12_instanced_draw_execution_status_name(
               d3d12_instanced_draw_execution.status)
        << " d3d12_instanced_draw_execution_ready=" << (d3d12_instanced_draw_execution.ready ? 1 : 0)
        << " d3d12_instanced_draw_execution_selected="
        << (d3d12_instanced_draw_execution.d3d12_backend_selected ? 1 : 0)
        << " d3d12_instanced_draw_execution_expected_instances="
        << d3d12_instanced_draw_execution.expected_instances_submitted
        << " d3d12_instanced_draw_calls=" << d3d12_instanced_draw_execution.instanced_draw_calls
        << " d3d12_instanced_indexed_draw_calls=" << d3d12_instanced_draw_execution.instanced_indexed_draw_calls
        << " d3d12_instanced_instances_submitted=" << d3d12_instanced_draw_execution.instanced_instances_submitted
        << " d3d12_instanced_draws_ok=" << (d3d12_instanced_draw_execution.instanced_draws_current ? 1 : 0)
        << " d3d12_instanced_instances_ok=" << (d3d12_instanced_draw_execution.instanced_instances_current ? 1 : 0)
        << " vulkan_instanced_draw_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_instanced_draw_execution_status_name(
               vulkan_instanced_draw_execution.status)
        << " vulkan_instanced_draw_execution_ready=" << (vulkan_instanced_draw_execution.ready ? 1 : 0)
        << " vulkan_instanced_draw_execution_selected="
        << (vulkan_instanced_draw_execution.vulkan_backend_selected ? 1 : 0)
        << " vulkan_instanced_draw_execution_expected_instances="
        << vulkan_instanced_draw_execution.expected_instances_submitted
        << " vulkan_instanced_draw_calls=" << vulkan_instanced_draw_execution.instanced_draw_calls
        << " vulkan_instanced_indexed_draw_calls=" << vulkan_instanced_draw_execution.instanced_indexed_draw_calls
        << " vulkan_instanced_instances_submitted=" << vulkan_instanced_draw_execution.instanced_instances_submitted
        << " vulkan_instanced_draws_ok=" << (vulkan_instanced_draw_execution.instanced_draws_current ? 1 : 0)
        << " vulkan_instanced_instances_ok=" << (vulkan_instanced_draw_execution.instanced_instances_current ? 1 : 0)
        << " rhi_instanced_draw_calls=" << report.rhi_instanced_draw_calls
        << " rhi_instanced_indexed_draw_calls=" << report.rhi_instanced_indexed_draw_calls
        << " rhi_instanced_instances_submitted=" << report.rhi_instanced_instances_submitted
        << " directional_shadow_status="
        << mirakana::win32_desktop_presentation_directional_shadow_status_name(report.directional_shadow_status)
        << " directional_shadow_requested=" << (report.directional_shadow_requested ? 1 : 0)
        << " directional_shadow_ready=" << (report.directional_shadow_ready ? 1 : 0)
        << " directional_shadow_filter_mode="
        << mirakana::win32_desktop_presentation_directional_shadow_filter_mode_name(
               report.directional_shadow_filter_mode)
        << " directional_shadow_filter_taps=" << report.directional_shadow_filter_tap_count
        << " directional_shadow_filter_radius_texels=" << report.directional_shadow_filter_radius_texels
        << " directional_shadow_cascade_count=" << report.directional_shadow_cascade_count
        << " directional_shadow_cascade_tile_width=" << report.directional_shadow_cascade_tile_width
        << " directional_shadow_atlas_width=" << report.directional_shadow_atlas_width
        << " directional_shadow_atlas_height=" << report.directional_shadow_atlas_height
        << " directional_shadow_light_space_cascades=" << report.directional_shadow_light_space_cascades
        << " directional_shadow_cascade_splits=" << report.directional_shadow_cascade_splits
        << " d3d12_shadow_cascade_policy_ready="
        << (options.require_d3d12_shadow_cascade_policy && d3d12_shadow_cascade_policy_ready(report) ? 1 : 0)
        << " d3d12_shadow_cascade_policy_selected="
        << (options.require_d3d12_shadow_cascade_policy &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12
                ? 1
                : 0)
        << " vulkan_shadow_cascade_policy_ready="
        << (options.require_vulkan_shadow_cascade_policy && vulkan_shadow_cascade_policy_ready(report) ? 1 : 0)
        << " vulkan_shadow_cascade_policy_selected="
        << (options.require_vulkan_shadow_cascade_policy &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan
                ? 1
                : 0)
        << " lighting_shadow_policy_status="
        << lighting_shadow_policy_status_name(options.require_lighting_shadow_policy, lighting_shadow_policy_ready)
        << " lighting_shadow_policy_ready=" << (lighting_shadow_policy_ready ? 1 : 0)
        << " lighting_shadow_policy_diagnostics=" << game.lighting_shadow_policy_diagnostics()
        << " lighting_shadow_policy_lights=" << game.lighting_shadow_policy_light_count()
        << " lighting_shadow_policy_directional_lights=" << game.lighting_shadow_policy_directional_light_count()
        << " lighting_shadow_policy_point_lights=" << game.lighting_shadow_policy_point_light_count()
        << " lighting_shadow_policy_spot_lights=" << game.lighting_shadow_policy_spot_light_count()
        << " lighting_shadow_policy_shadowed_lights=" << game.lighting_shadow_policy_shadowed_light_count()
        << " lighting_shadow_policy_directional_cascades=" << game.lighting_shadow_policy_directional_cascade_count()
        << " lighting_shadow_policy_shadow_atlas_width=" << game.lighting_shadow_policy_shadow_atlas_width()
        << " lighting_shadow_policy_shadow_atlas_height=" << game.lighting_shadow_policy_shadow_atlas_height()
        << " lighting_shadow_policy_light_rows=" << game.lighting_shadow_policy_light_rows()
        << " lighting_shadow_policy_ready_frames=" << game.lighting_shadow_policy_ready_frames()
        << " scene_scale_policy_status="
        << mirakana::win32_desktop_presentation_scene_scale_policy_status_name(scene_scale_policy.status)
        << " scene_scale_policy_ready=" << (scene_scale_policy.ready ? 1 : 0)
        << " scene_scale_policy_diagnostics=" << scene_scale_policy.diagnostics_count
        << " scene_scale_policy_scene_resources_ready=" << (scene_scale_policy.scene_resources_ready ? 1 : 0)
        << " scene_scale_policy_expected_frames=" << scene_scale_policy.expected_frames
        << " scene_scale_policy_frames_finished=" << scene_scale_policy.frames_finished
        << " scene_scale_policy_frames_current=" << (scene_scale_policy.frames_current ? 1 : 0)
        << " scene_scale_policy_draw_groups=" << scene_scale_policy.draw_group_count
        << " scene_scale_policy_static_mesh_groups=" << scene_scale_policy.static_mesh_draw_groups
        << " scene_scale_policy_skinned_mesh_groups=" << scene_scale_policy.skinned_mesh_draw_groups
        << " scene_scale_policy_morph_mesh_groups=" << scene_scale_policy.morph_mesh_draw_groups
        << " scene_scale_policy_sprite_groups=" << scene_scale_policy.sprite_draw_groups
        << " scene_scale_policy_requested_instances=" << scene_scale_policy.requested_instance_count
        << " scene_scale_policy_visible_instances=" << scene_scale_policy.visible_instance_count
        << " scene_scale_policy_culled_instances=" << scene_scale_policy.culled_instance_count
        << " scene_scale_policy_draw_calls=" << scene_scale_policy.draw_call_count
        << " scene_scale_policy_instanced_draw_calls=" << scene_scale_policy.instanced_draw_call_count
        << " scene_scale_policy_instanced_visible_instances=" << scene_scale_policy.instanced_visible_instance_count
        << " scene_scale_policy_lod_groups=" << scene_scale_policy.lod_group_count
        << " scene_scale_policy_cpu_culling_groups=" << scene_scale_policy.cpu_culling_group_count
        << " scene_scale_policy_backend_instancing_evidence_required="
        << (scene_scale_policy.backend_instancing_evidence_required ? 1 : 0)
        << " scene_scale_policy_backend_instancing_evidence_ready="
        << (scene_scale_policy.backend_instancing_evidence_ready ? 1 : 0)
        << " scene_scale_policy_performance_measurement_required="
        << (scene_scale_policy.performance_measurement_required ? 1 : 0)
        << " scene_scale_policy_performance_measurement_ready="
        << (scene_scale_policy.performance_measurement_ready ? 1 : 0) << " gpu_memory_policy_status="
        << mirakana::win32_desktop_presentation_gpu_memory_policy_status_name(gpu_memory_policy.status)
        << " gpu_memory_policy_ready=" << (gpu_memory_policy.ready ? 1 : 0)
        << " gpu_memory_policy_diagnostics=" << gpu_memory_policy.diagnostics_count
        << " gpu_memory_policy_scene_resources_ready=" << (gpu_memory_policy.scene_resources_ready ? 1 : 0)
        << " gpu_memory_policy_expected_frames=" << gpu_memory_policy.expected_frames
        << " gpu_memory_policy_frames_finished=" << gpu_memory_policy.frames_finished
        << " gpu_memory_policy_frames_current=" << (gpu_memory_policy.frames_current ? 1 : 0)
        << " gpu_memory_policy_requests=" << gpu_memory_policy.request_count
        << " gpu_memory_policy_requested_bytes=" << gpu_memory_policy.total_requested_bytes
        << " gpu_memory_policy_counted_bytes=" << gpu_memory_policy.total_counted_bytes
        << " gpu_memory_policy_os_local_budget_bytes=" << gpu_memory_policy.os_local_budget_bytes
        << " gpu_memory_policy_os_local_usage_bytes=" << gpu_memory_policy.os_local_usage_bytes
        << " gpu_memory_policy_committed_byte_estimate=" << gpu_memory_policy.committed_byte_estimate
        << " gpu_memory_policy_transient_heap_allocations=" << gpu_memory_policy.transient_heap_allocations
        << " gpu_memory_policy_transient_placed_allocations=" << gpu_memory_policy.transient_placed_allocations
        << " gpu_memory_policy_transient_placed_resources_alive=" << gpu_memory_policy.transient_placed_resources_alive
        << " gpu_memory_policy_upload_bytes_written=" << gpu_memory_policy.upload_bytes_written
        << " gpu_memory_policy_transient_heap_requests=" << gpu_memory_policy.transient_heap_request_count
        << " gpu_memory_policy_upload_pressure_requests=" << gpu_memory_policy.upload_pressure_request_count
        << " gpu_memory_policy_declared_budget_requests=" << gpu_memory_policy.declared_budget_request_count
        << " gpu_memory_policy_residency_pressure_requests=" << gpu_memory_policy.residency_pressure_request_count
        << " gpu_memory_policy_package_counter_requests=" << gpu_memory_policy.package_counter_request_count
        << " gpu_memory_policy_memory_budget_evidence_ready="
        << (gpu_memory_policy.memory_budget_evidence_ready ? 1 : 0)
        << " gpu_memory_policy_residency_pressure_evidence_ready="
        << (gpu_memory_policy.residency_pressure_evidence_ready ? 1 : 0)
        << " gpu_memory_policy_package_counter_evidence_ready="
        << (gpu_memory_policy.package_counter_evidence_ready ? 1 : 0)
        << " gpu_memory_policy_residency_pressure_events=" << gpu_memory_policy.residency_pressure_event_count
        << " gpu_memory_policy_backend_memory_evidence_required="
        << (gpu_memory_policy.backend_memory_evidence_required ? 1 : 0)
        << " gpu_memory_policy_backend_memory_evidence_ready="
        << (gpu_memory_policy.backend_memory_evidence_ready ? 1 : 0)
        << " gpu_memory_policy_os_video_memory_budget_required="
        << (gpu_memory_policy.os_video_memory_budget_required ? 1 : 0)
        << " gpu_memory_policy_os_video_memory_budget_available="
        << (gpu_memory_policy.os_video_memory_budget_available ? 1 : 0)
        << " memory_diagnostics_status=" << mirakana::memory_diagnostics_status_label(memory_diagnostics.status)
        << " memory_diagnostics_ready="
        << (memory_diagnostics.status == mirakana::MemoryDiagnosticsStatus::ready ? 1 : 0)
        << " memory_diagnostics_rows=" << memory_diagnostics.row_count
        << " memory_diagnostics_classes=" << memory_diagnostics.class_summaries.size()
        << " memory_diagnostics_total_bytes=" << memory_diagnostics.total_bytes
        << " memory_diagnostics_high_water_bytes=" << memory_diagnostics.high_water_bytes
        << " memory_diagnostics_total_allocation_count=" << memory_diagnostics.total_allocation_count
        << " memory_diagnostics_diagnostics=" << memory_diagnostics.diagnostics.size()
        << " memory_diagnostics_budgeted_classes=" << memory_diagnostics_budgeted_class_count(memory_diagnostics)
        << " memory_diagnostics_budget_pressure_classes="
        << memory_diagnostics_pressure_class_count(memory_diagnostics, mirakana::MemoryBudgetPressure::warning)
        << " memory_diagnostics_budget_exceeded_classes="
        << memory_diagnostics_pressure_class_count(memory_diagnostics, mirakana::MemoryBudgetPressure::exceeded)
        << " memory_diagnostics_invalid_counter="
        << (memory_diagnostic_code_present(memory_diagnostics, mirakana::MemoryDiagnosticsCode::invalid_counter) ? 1
                                                                                                                 : 0)
        << " memory_diagnostics_stale_generation="
        << (memory_diagnostic_code_present(memory_diagnostics, mirakana::MemoryDiagnosticsCode::stale_generation) ? 1
                                                                                                                  : 0)
        << " memory_diagnostics_use_after_safe_point="
        << (memory_diagnostic_code_present(memory_diagnostics, mirakana::MemoryDiagnosticsCode::use_after_safe_point)
                ? 1
                : 0)
        << " memory_diagnostics_package_resident_cpu_bytes="
        << memory_class_bytes(memory_diagnostics, mirakana::MemoryLifetimeClass::package_resident_cpu)
        << " memory_diagnostics_package_resident_cpu_allocations="
        << memory_class_allocations(memory_diagnostics, mirakana::MemoryLifetimeClass::package_resident_cpu)
        << " memory_diagnostics_resident_gpu_bytes="
        << memory_class_bytes(memory_diagnostics, mirakana::MemoryLifetimeClass::resident_gpu)
        << " memory_diagnostics_resident_gpu_allocations="
        << memory_class_allocations(memory_diagnostics, mirakana::MemoryLifetimeClass::resident_gpu)
        << " memory_diagnostics_resident_gpu_budget_bytes="
        << memory_class_budget(memory_diagnostics, mirakana::MemoryLifetimeClass::resident_gpu)
        << " memory_diagnostics_resident_gpu_pressure="
        << memory_class_pressure(memory_diagnostics, mirakana::MemoryLifetimeClass::resident_gpu)
        << " memory_diagnostics_upload_staging_bytes="
        << memory_class_bytes(memory_diagnostics, mirakana::MemoryLifetimeClass::upload_staging)
        << " memory_diagnostics_upload_staging_allocations="
        << memory_class_allocations(memory_diagnostics, mirakana::MemoryLifetimeClass::upload_staging)
        << " memory_diagnostics_transient_gpu_allocations="
        << memory_class_allocations(memory_diagnostics, mirakana::MemoryLifetimeClass::transient_gpu)
        << " memory_diagnostics_transient_gpu_aliasing_barriers="
        << memory_diagnostics_transient_gpu_aliasing_barriers(framegraph_multiqueue_requested, framegraph_multiqueue)
        << " memory_diagnostics_transient_gpu_framegraph_aliasing_ready="
        << (memory_diagnostics_transient_gpu_framegraph_aliasing_ready(framegraph_multiqueue_requested,
                                                                       framegraph_multiqueue)
                ? 1
                : 0)
        << " d3d12_gpu_memory_execution_status="
        << mirakana::win32_desktop_presentation_d3d12_gpu_memory_execution_status_name(
               d3d12_gpu_memory_execution.status)
        << " d3d12_gpu_memory_execution_ready=" << (d3d12_gpu_memory_execution.ready ? 1 : 0)
        << " d3d12_gpu_memory_execution_selected=" << (d3d12_gpu_memory_execution.d3d12_backend_selected ? 1 : 0)
        << " d3d12_gpu_memory_execution_os_video_memory_budget_available="
        << (d3d12_gpu_memory_execution.os_video_memory_budget_available ? 1 : 0)
        << " d3d12_gpu_memory_execution_committed_byte_estimate_available="
        << (d3d12_gpu_memory_execution.committed_byte_estimate_available ? 1 : 0)
        << " d3d12_gpu_memory_execution_local_video_memory_budget_bytes="
        << d3d12_gpu_memory_execution.local_video_memory_budget_bytes
        << " d3d12_gpu_memory_execution_local_video_memory_usage_bytes="
        << d3d12_gpu_memory_execution.local_video_memory_usage_bytes
        << " d3d12_gpu_memory_execution_committed_resources_byte_estimate="
        << d3d12_gpu_memory_execution.committed_resources_byte_estimate
        << " d3d12_gpu_memory_execution_transient_heap_allocations="
        << d3d12_gpu_memory_execution.transient_heap_allocations
        << " d3d12_gpu_memory_execution_transient_placed_allocations="
        << d3d12_gpu_memory_execution.transient_placed_allocations
        << " d3d12_gpu_memory_execution_transient_placed_resources_alive="
        << d3d12_gpu_memory_execution.transient_placed_resources_alive
        << " d3d12_gpu_memory_execution_upload_bytes_written=" << d3d12_gpu_memory_execution.upload_bytes_written
        << " d3d12_gpu_memory_execution_budget_ok=" << (d3d12_gpu_memory_execution.memory_budget_current ? 1 : 0)
        << " d3d12_gpu_memory_execution_transient_heap_ok="
        << (d3d12_gpu_memory_execution.transient_heap_current ? 1 : 0) << " vulkan_gpu_memory_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_gpu_memory_execution_status_name(
               vulkan_gpu_memory_execution.status)
        << " vulkan_gpu_memory_execution_ready=" << (vulkan_gpu_memory_execution.ready ? 1 : 0)
        << " vulkan_gpu_memory_execution_selected=" << (vulkan_gpu_memory_execution.vulkan_backend_selected ? 1 : 0)
        << " vulkan_gpu_memory_execution_committed_byte_estimate_available="
        << (vulkan_gpu_memory_execution.committed_byte_estimate_available ? 1 : 0)
        << " vulkan_gpu_memory_execution_committed_resources_byte_estimate="
        << vulkan_gpu_memory_execution.committed_resources_byte_estimate
        << " vulkan_gpu_memory_execution_transient_heap_allocations="
        << vulkan_gpu_memory_execution.transient_heap_allocations
        << " vulkan_gpu_memory_execution_transient_placed_allocations="
        << vulkan_gpu_memory_execution.transient_placed_allocations
        << " vulkan_gpu_memory_execution_transient_placed_resources_alive="
        << vulkan_gpu_memory_execution.transient_placed_resources_alive
        << " vulkan_gpu_memory_execution_upload_bytes_written=" << vulkan_gpu_memory_execution.upload_bytes_written
        << " vulkan_gpu_memory_execution_framegraph_barrier_steps_executed="
        << vulkan_gpu_memory_execution.framegraph_barrier_steps_executed
        << " vulkan_gpu_memory_execution_budget_ok=" << (vulkan_gpu_memory_execution.memory_budget_current ? 1 : 0)
        << " vulkan_gpu_memory_execution_transient_heap_ok="
        << (vulkan_gpu_memory_execution.transient_heap_current ? 1 : 0) << " debug_profiling_policy_status="
        << mirakana::win32_desktop_presentation_debug_profiling_policy_status_name(debug_profiling_policy.status)
        << " debug_profiling_policy_ready=" << (debug_profiling_policy.ready ? 1 : 0)
        << " debug_profiling_policy_diagnostics=" << debug_profiling_policy.diagnostics_count
        << " debug_profiling_policy_scene_resources_ready=" << (debug_profiling_policy.scene_resources_ready ? 1 : 0)
        << " debug_profiling_policy_expected_frames=" << debug_profiling_policy.expected_frames
        << " debug_profiling_policy_frames_finished=" << debug_profiling_policy.frames_finished
        << " debug_profiling_policy_frames_current=" << (debug_profiling_policy.frames_current ? 1 : 0)
        << " debug_profiling_policy_requests=" << debug_profiling_policy.request_count
        << " debug_profiling_policy_gpu_timestamp_ticks_per_second="
        << debug_profiling_policy.gpu_timestamp_ticks_per_second
        << " debug_profiling_policy_gpu_debug_scopes_begun=" << debug_profiling_policy.gpu_debug_scopes_begun
        << " debug_profiling_policy_gpu_debug_scopes_ended=" << debug_profiling_policy.gpu_debug_scopes_ended
        << " debug_profiling_policy_gpu_debug_markers_inserted=" << debug_profiling_policy.gpu_debug_markers_inserted
        << " debug_profiling_policy_framegraph_barrier_steps_executed="
        << debug_profiling_policy.framegraph_barrier_steps_executed
        << " debug_profiling_policy_framegraph_render_passes_recorded="
        << debug_profiling_policy.framegraph_render_passes_recorded
        << " debug_profiling_policy_gpu_timestamp_requests=" << debug_profiling_policy.gpu_timestamp_request_count
        << " debug_profiling_policy_gpu_debug_marker_requests=" << debug_profiling_policy.gpu_debug_marker_request_count
        << " debug_profiling_policy_capture_handoff_requests=" << debug_profiling_policy.capture_handoff_request_count
        << " debug_profiling_policy_cpu_profile_zones=" << debug_profiling_policy.cpu_profile_zone_count
        << " debug_profiling_policy_trace_capture_handoff_rows="
        << debug_profiling_policy.trace_capture_handoff_row_count
        << " debug_profiling_policy_cpu_profile_zone_requests=" << debug_profiling_policy.cpu_profile_zone_request_count
        << " debug_profiling_policy_trace_capture_handoff_requests="
        << debug_profiling_policy.trace_capture_handoff_request_count
        << " debug_profiling_policy_package_counter_requests=" << debug_profiling_policy.package_counter_request_count
        << " debug_profiling_policy_cpu_profile_zone_evidence_ready="
        << (debug_profiling_policy.cpu_profile_zone_evidence_ready ? 1 : 0)
        << " debug_profiling_policy_trace_capture_handoff_evidence_ready="
        << (debug_profiling_policy.trace_capture_handoff_evidence_ready ? 1 : 0)
        << " debug_profiling_policy_package_counter_evidence_ready="
        << (debug_profiling_policy.package_counter_evidence_ready ? 1 : 0)
        << " debug_profiling_policy_backend_profiling_evidence_required="
        << (debug_profiling_policy.backend_profiling_evidence_required ? 1 : 0)
        << " debug_profiling_policy_backend_profiling_evidence_ready="
        << (debug_profiling_policy.backend_profiling_evidence_ready ? 1 : 0)
        << " d3d12_debug_profiling_execution_status="
        << mirakana::win32_desktop_presentation_d3d12_debug_profiling_execution_status_name(
               d3d12_debug_profiling_execution.status)
        << " d3d12_debug_profiling_execution_ready=" << (d3d12_debug_profiling_execution.ready ? 1 : 0)
        << " d3d12_debug_profiling_execution_selected="
        << (d3d12_debug_profiling_execution.d3d12_backend_selected ? 1 : 0)
        << " d3d12_debug_profiling_execution_gpu_timestamp_ticks_per_second="
        << d3d12_debug_profiling_execution.gpu_timestamp_ticks_per_second
        << " d3d12_debug_profiling_execution_gpu_debug_scopes_begun="
        << d3d12_debug_profiling_execution.gpu_debug_scopes_begun
        << " d3d12_debug_profiling_execution_gpu_debug_scopes_ended="
        << d3d12_debug_profiling_execution.gpu_debug_scopes_ended
        << " d3d12_debug_profiling_execution_gpu_debug_markers_inserted="
        << d3d12_debug_profiling_execution.gpu_debug_markers_inserted
        << " d3d12_debug_profiling_execution_framegraph_barrier_steps_executed="
        << d3d12_debug_profiling_execution.framegraph_barrier_steps_executed
        << " d3d12_debug_profiling_execution_framegraph_render_passes_recorded="
        << d3d12_debug_profiling_execution.framegraph_render_passes_recorded
        << " d3d12_debug_profiling_execution_gpu_timestamps_ok="
        << (d3d12_debug_profiling_execution.gpu_timestamps_current ? 1 : 0)
        << " d3d12_debug_profiling_execution_gpu_debug_markers_ok="
        << (d3d12_debug_profiling_execution.gpu_debug_markers_current ? 1 : 0)
        << " d3d12_debug_profiling_execution_frame_diagnostics_ok="
        << (d3d12_debug_profiling_execution.frame_diagnostics_current ? 1 : 0)
        << " vulkan_debug_profiling_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_debug_profiling_execution_status_name(
               vulkan_debug_profiling_execution.status)
        << " vulkan_debug_profiling_execution_ready=" << (vulkan_debug_profiling_execution.ready ? 1 : 0)
        << " vulkan_debug_profiling_execution_selected="
        << (vulkan_debug_profiling_execution.vulkan_backend_selected ? 1 : 0)
        << " vulkan_debug_profiling_execution_gpu_timestamp_ticks_per_second="
        << vulkan_debug_profiling_execution.gpu_timestamp_ticks_per_second
        << " vulkan_debug_profiling_execution_gpu_debug_scopes_begun="
        << vulkan_debug_profiling_execution.gpu_debug_scopes_begun
        << " vulkan_debug_profiling_execution_gpu_debug_scopes_ended="
        << vulkan_debug_profiling_execution.gpu_debug_scopes_ended
        << " vulkan_debug_profiling_execution_gpu_debug_markers_inserted="
        << vulkan_debug_profiling_execution.gpu_debug_markers_inserted
        << " vulkan_debug_profiling_execution_framegraph_barrier_steps_executed="
        << vulkan_debug_profiling_execution.framegraph_barrier_steps_executed
        << " vulkan_debug_profiling_execution_framegraph_render_passes_recorded="
        << vulkan_debug_profiling_execution.framegraph_render_passes_recorded
        << " vulkan_debug_profiling_execution_gpu_timestamps_ok="
        << (vulkan_debug_profiling_execution.gpu_timestamps_current ? 1 : 0)
        << " vulkan_debug_profiling_execution_gpu_debug_markers_ok="
        << (vulkan_debug_profiling_execution.gpu_debug_markers_current ? 1 : 0)
        << " vulkan_debug_profiling_execution_frame_diagnostics_ok="
        << (vulkan_debug_profiling_execution.frame_diagnostics_current ? 1 : 0) << " job_scheduling_evidence_status="
        << job_scheduling_evidence_status_name(options.require_job_scheduling_evidence, job_scheduling_evidence)
        << " job_scheduling_evidence_ready=" << (job_scheduling_evidence_ready(job_scheduling_evidence) ? 1 : 0)
        << " job_scheduling_evidence_diagnostics=" << job_scheduling_evidence.scheduling_summary.diagnostics.size()
        << " job_scheduling_evidence_scratch_diagnostics=" << job_scheduling_evidence.scratch_summary.diagnostics.size()
        << " job_scheduling_evidence_topology_rows="
        << job_scheduling_evidence.scheduling_summary.worker_topology_row_count
        << " job_scheduling_evidence_worker_count=" << job_scheduling_evidence.scheduling_summary.worker_count
        << " job_scheduling_evidence_queue_count=" << job_scheduling_evidence.scheduling_summary.queue_count
        << " job_scheduling_evidence_queue_rows=" << job_scheduling_evidence.queue_rows.size()
        << " job_scheduling_evidence_submitted_jobs=" << job_scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " job_scheduling_evidence_completed_jobs=" << job_scheduling_evidence.scheduling_summary.total_completed_jobs
        << " job_scheduling_evidence_execution_rows=" << job_scheduling_evidence.execution_order.size()
        << " job_scheduling_evidence_deterministic_merges="
        << job_scheduling_evidence.scheduling_summary.total_deterministic_merge_count
        << " job_scheduling_evidence_nondeterministic_merges="
        << job_scheduling_evidence.scheduling_summary.total_nondeterministic_merge_count
        << " job_scheduling_evidence_blocked_dependencies="
        << job_scheduling_evidence.scheduling_summary.total_blocked_dependency_count
        << " job_scheduling_evidence_dependency_cycles="
        << job_scheduling_evidence.scheduling_summary.total_dependency_cycle_count
        << " job_scheduling_evidence_queue_overflows="
        << job_scheduling_evidence.scheduling_summary.total_queue_overflow_count
        << " job_scheduling_evidence_scratch_misuse="
        << job_scheduling_evidence.scheduling_summary.total_scratch_misuse_count
        << " job_scheduling_evidence_scratch_rows=" << job_scheduling_evidence.worker_scratch_rows.size()
        << " job_scheduling_evidence_scratch_bytes=" << job_scheduling_evidence.scratch_summary.total_bytes
        << " job_scheduling_evidence_scratch_high_water_bytes="
        << job_scheduling_evidence.scratch_summary.high_water_bytes
        << " job_scheduling_evidence_native_threads_started=0"
        << " job_scheduling_evidence_thread_pool_started=0"
        << " job_scheduling_evidence_affinity_policy_applied=0"
        << " job_scheduling_evidence_numa_policy_applied=0"
        << " job_scheduling_evidence_simd_dispatch_applied=0"
        << " job_scheduling_evidence_gpu_async_overlap_applied=0"
        << " job_execution_topology_policy_status="
        << job_execution_topology_policy_status_name(options.require_job_execution_topology_policy,
                                                     job_execution_topology_policy)
        << " job_execution_topology_policy_ready="
        << (job_execution_topology_policy_ready(job_execution_topology_policy) ? 1 : 0)
        << " job_execution_topology_policy_diagnostics=" << job_execution_topology_policy.diagnostics.size()
        << " job_execution_topology_policy_observed_logical_processors="
        << job_execution_topology_policy.observed_logical_processor_count
        << " job_execution_topology_policy_effective_logical_processors="
        << job_execution_topology_policy.effective_logical_processor_count
        << " job_execution_topology_policy_selected_worker_count="
        << job_execution_topology_policy.selected_worker_count
        << " job_execution_topology_policy_worker_count_limit=" << job_execution_topology_policy.worker_count_limit
        << " job_execution_topology_policy_reserved_logical_processors="
        << job_execution_topology_policy.reserved_logical_processor_count
        << " job_execution_topology_policy_fallback_used="
        << (job_execution_topology_policy.hardware_concurrency_fallback_used ? 1 : 0)
        << " job_execution_topology_policy_worker_count_limited_by_cap="
        << (job_execution_topology_policy.worker_count_limited_by_cap ? 1 : 0)
        << " job_execution_topology_policy_requested_worker_count_used="
        << (job_execution_topology_policy.requested_worker_count_used ? 1 : 0)
        << " job_execution_topology_policy_clamped_to_logical_processors="
        << (job_execution_topology_policy.worker_count_clamped_to_logical_processors ? 1 : 0)
        << " job_execution_topology_policy_processor_group_count="
        << job_execution_topology_policy.topology_row.processor_group_count
        << " job_execution_topology_policy_numa_node_count="
        << job_execution_topology_policy.topology_row.numa_node_count
        << " job_execution_topology_policy_processor_group_policy_applied=0"
        << " job_execution_topology_policy_numa_policy_applied=0"
        << " job_execution_topology_policy_affinity_policy_applied=0"
        << " job_execution_topology_policy_simd_dispatch_applied=0"
        << " job_execution_topology_policy_gpu_async_overlap_applied=0"
        << " job_execution_topology_policy_cuda_path_used=0"
        << " job_execution_topology_policy_hip_path_used=0"
        << " job_execution_topology_policy_sycl_path_used=0"
        << " job_execution_foundation_status=" << job_execution_foundation_status_name(job_execution_foundation)
        << " job_execution_foundation_ready=" << (job_execution_foundation_ready(job_execution_foundation) ? 1 : 0)
        << " job_execution_foundation_pool_status="
        << mirakana::job_execution_pool_status_label(job_execution_foundation.pool_status)
        << " job_execution_foundation_run_status="
        << mirakana::job_execution_run_status_label(job_execution_foundation.run_result.status)
        << " job_execution_foundation_diagnostics="
        << job_execution_foundation_diagnostic_count(job_execution_foundation)
        << " job_execution_foundation_worker_threads_started="
        << job_execution_foundation.run_result.worker_threads_started << " job_execution_foundation_tasks_submitted="
        << job_execution_foundation.run_result.scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " job_execution_foundation_tasks_executed=" << job_execution_foundation.run_result.tasks_executed
        << " job_execution_foundation_tasks_failed=" << job_execution_foundation.run_result.tasks_failed
        << " job_execution_foundation_task_side_effects=" << job_execution_foundation.task_side_effects
        << " job_execution_foundation_execution_rows="
        << job_execution_foundation.run_result.scheduling_evidence.execution_order.size()
        << " job_execution_foundation_queue_rows="
        << job_execution_foundation.run_result.scheduling_evidence.queue_rows.size()
        << " job_execution_foundation_deterministic_merges="
        << job_execution_foundation.run_result.scheduling_evidence.scheduling_summary.total_deterministic_merge_count
        << " job_execution_foundation_scratch_rows="
        << job_execution_foundation.run_result.scheduling_evidence.worker_scratch_rows.size()
        << " job_execution_foundation_scratch_bytes="
        << job_execution_foundation.run_result.scheduling_evidence.scratch_summary.total_bytes
        << " job_execution_foundation_scratch_high_water_bytes="
        << job_execution_foundation.run_result.scheduling_evidence.scratch_summary.high_water_bytes
        << " job_execution_foundation_work_stealing_applied=0"
        << " job_execution_foundation_affinity_policy_applied=0"
        << " job_execution_foundation_numa_policy_applied=0"
        << " job_execution_foundation_simd_dispatch_applied=0"
        << " job_execution_foundation_gpu_async_overlap_applied=0"
        << " job_execution_foundation_cuda_path_used=0"
        << " job_execution_foundation_hip_path_used=0"
        << " job_execution_foundation_sycl_path_used=0"
        << " job_execution_work_stealing_status="
        << job_execution_work_stealing_status_name(job_execution_work_stealing) << " job_execution_work_stealing_ready="
        << (job_execution_work_stealing_ready(job_execution_work_stealing) ? 1 : 0)
        << " job_execution_work_stealing_pool_status="
        << mirakana::job_execution_pool_status_label(job_execution_work_stealing.pool_status)
        << " job_execution_work_stealing_run_status="
        << mirakana::job_execution_run_status_label(job_execution_work_stealing.run_result.status)
        << " job_execution_work_stealing_diagnostics="
        << job_execution_work_stealing_diagnostic_count(job_execution_work_stealing)
        << " job_execution_work_stealing_worker_threads_started="
        << job_execution_work_stealing.run_result.worker_threads_started
        << " job_execution_work_stealing_tasks_submitted="
        << job_execution_work_stealing.run_result.scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " job_execution_work_stealing_tasks_executed=" << job_execution_work_stealing.run_result.tasks_executed
        << " job_execution_work_stealing_tasks_failed=" << job_execution_work_stealing.run_result.tasks_failed
        << " job_execution_work_stealing_task_side_effects=" << job_execution_work_stealing.task_side_effects
        << " job_execution_work_stealing_execution_rows="
        << job_execution_work_stealing.run_result.scheduling_evidence.execution_order.size()
        << " job_execution_work_stealing_queue_rows="
        << job_execution_work_stealing.run_result.scheduling_evidence.queue_rows.size()
        << " job_execution_work_stealing_deterministic_merges="
        << job_execution_work_stealing.run_result.scheduling_evidence.scheduling_summary.total_deterministic_merge_count
        << " job_execution_work_stealing_steal_attempts=" << job_execution_work_stealing.run_result.steal_attempt_count
        << " job_execution_work_stealing_steal_successes=" << job_execution_work_stealing.run_result.steal_success_count
        << " job_execution_work_stealing_worker_waits=" << job_execution_work_stealing.run_result.worker_wait_count
        << " job_execution_work_stealing_scratch_rows="
        << job_execution_work_stealing.run_result.scheduling_evidence.worker_scratch_rows.size()
        << " job_execution_work_stealing_scratch_bytes="
        << job_execution_work_stealing.run_result.scheduling_evidence.scratch_summary.total_bytes
        << " job_execution_work_stealing_scratch_high_water_bytes="
        << job_execution_work_stealing.run_result.scheduling_evidence.scratch_summary.high_water_bytes
        << " job_execution_work_stealing_applied="
        << (job_execution_work_stealing.run_result.work_stealing_applied ? 1 : 0)
        << " job_execution_work_stealing_affinity_policy_applied=0"
        << " job_execution_work_stealing_numa_policy_applied=0"
        << " job_execution_work_stealing_simd_dispatch_applied=0"
        << " job_execution_work_stealing_gpu_async_overlap_applied=0"
        << " job_execution_work_stealing_cuda_path_used=0"
        << " job_execution_work_stealing_hip_path_used=0"
        << " job_execution_work_stealing_sycl_path_used=0"
        << " job_execution_placement_policy_status="
        << job_execution_placement_policy_status_name(job_execution_placement_policy)
        << " job_execution_placement_policy_ready="
        << (job_execution_placement_policy_ready(job_execution_placement_policy) ? 1 : 0)
        << " job_execution_placement_policy_requested_mode="
        << mirakana::job_execution_placement_policy_mode_label(
               job_execution_placement_policy.ready_policy.requested_mode)
        << " job_execution_placement_policy_selected_mode="
        << mirakana::job_execution_placement_policy_mode_label(
               job_execution_placement_policy.ready_policy.selected_mode)
        << " job_execution_placement_policy_diagnostics="
        << job_execution_placement_policy.ready_policy.diagnostics.size()
        << " job_execution_placement_policy_host_evidence_required_diagnostics="
        << job_execution_placement_policy.host_evidence_policy.diagnostic_codes.size()
        << " job_execution_placement_policy_inherited_worker_count="
        << job_execution_placement_policy.ready_policy.inherited_worker_count
        << " job_execution_placement_policy_inherited_work_stealing_enabled="
        << (job_execution_placement_policy.ready_policy.pool_desc.work_stealing_enabled ? 1 : 0)
        << " job_execution_placement_policy_numa_node_count="
        << job_execution_placement_policy.ready_policy.numa_node_count
        << " job_execution_placement_policy_performance_core_count="
        << job_execution_placement_policy.ready_policy.performance_core_count
        << " job_execution_placement_policy_efficiency_core_count="
        << job_execution_placement_policy.ready_policy.efficiency_core_count
        << " job_execution_placement_policy_smt_sibling_topology_known="
        << (job_execution_placement_policy.ready_policy.smt_sibling_topology_known ? 1 : 0)
        << " job_execution_placement_policy_affinity_policy_applied="
        << (job_execution_placement_policy.ready_policy.affinity_policy_applied ? 1 : 0)
        << " job_execution_placement_policy_numa_policy_applied="
        << (job_execution_placement_policy.ready_policy.numa_policy_applied ? 1 : 0)
        << " job_execution_placement_policy_simd_dispatch_applied="
        << (job_execution_placement_policy.ready_policy.simd_dispatch_applied ? 1 : 0)
        << " job_execution_placement_policy_gpu_async_overlap_applied="
        << (job_execution_placement_policy.ready_policy.gpu_async_overlap_applied ? 1 : 0)
        << " job_execution_placement_policy_cuda_path_used=0"
        << " job_execution_placement_policy_hip_path_used=0"
        << " job_execution_placement_policy_sycl_path_used=0"
        << " windows_cpu_set_worker_placement_status="
        << windows_cpu_set_worker_placement_status_name(windows_cpu_set_worker_placement)
        << " windows_cpu_set_worker_placement_ready="
        << (windows_cpu_set_worker_placement_ready(windows_cpu_set_worker_placement) ? 1 : 0)
        << " windows_cpu_set_worker_placement_pool_status="
        << mirakana::job_execution_pool_status_label(windows_cpu_set_worker_placement.pool_status)
        << " windows_cpu_set_worker_placement_run_status="
        << mirakana::job_execution_run_status_label(windows_cpu_set_worker_placement.run_result.status)
        << " windows_cpu_set_worker_placement_diagnostics="
        << windows_cpu_set_worker_placement_diagnostic_count(windows_cpu_set_worker_placement)
        << " windows_cpu_set_worker_placement_topology_rows="
        << windows_cpu_set_worker_placement.run_result.scheduling_evidence.scheduling_summary.worker_topology_row_count
        << " windows_cpu_set_worker_placement_selected_cpu_sets="
        << windows_cpu_set_worker_placement.placement_plan.selected_cpu_set_count
        << " windows_cpu_set_worker_placement_worker_rows="
        << windows_cpu_set_worker_placement.placement_plan.worker_rows.size()
        << " windows_cpu_set_worker_placement_worker_threads_started="
        << windows_cpu_set_worker_placement.run_result.worker_threads_started
        << " windows_cpu_set_worker_placement_attempts="
        << windows_cpu_set_worker_placement.run_result.worker_placement_attempt_count
        << " windows_cpu_set_worker_placement_applied="
        << windows_cpu_set_worker_placement.run_result.worker_placement_applied_count
        << " windows_cpu_set_worker_placement_selected_cpu_set_applications="
        << windows_cpu_set_worker_placement.run_result.worker_placement_selected_cpu_set_count
        << " windows_cpu_set_worker_placement_tasks_submitted="
        << windows_cpu_set_worker_placement.run_result.scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " windows_cpu_set_worker_placement_tasks_executed="
        << windows_cpu_set_worker_placement.run_result.tasks_executed
        << " windows_cpu_set_worker_placement_tasks_failed=" << windows_cpu_set_worker_placement.run_result.tasks_failed
        << " windows_cpu_set_worker_placement_task_side_effects=" << windows_cpu_set_worker_placement.task_side_effects
        << " windows_cpu_set_worker_placement_native_thread_handles_exposed=0"
        << " windows_cpu_set_worker_placement_linux_affinity_applied=0"
        << " windows_cpu_set_worker_placement_numa_allocation_applied=0"
        << " windows_cpu_set_worker_placement_hybrid_smt_policy_applied=0"
        << " windows_cpu_set_worker_placement_simd_dispatch_applied=0"
        << " windows_cpu_set_worker_placement_gpu_async_overlap_applied=0"
        << " windows_cpu_set_worker_placement_cuda_path_used=0"
        << " windows_cpu_set_worker_placement_hip_path_used=0"
        << " windows_cpu_set_worker_placement_sycl_path_used=0"
        << " windows_cpu_set_smt_worker_placement_status="
        << windows_cpu_set_smt_worker_placement_status_name(windows_cpu_set_smt_worker_placement)
        << " windows_cpu_set_smt_worker_placement_ready="
        << (windows_cpu_set_smt_worker_placement_ready(windows_cpu_set_smt_worker_placement) ? 1 : 0)
        << " windows_cpu_set_smt_worker_placement_pool_status="
        << mirakana::job_execution_pool_status_label(windows_cpu_set_smt_worker_placement.pool_status)
        << " windows_cpu_set_smt_worker_placement_run_status="
        << mirakana::job_execution_run_status_label(windows_cpu_set_smt_worker_placement.run_result.status)
        << " windows_cpu_set_smt_worker_placement_diagnostics="
        << windows_cpu_set_worker_placement_diagnostic_count(windows_cpu_set_smt_worker_placement)
        << " windows_cpu_set_smt_worker_placement_selected_mode="
        << mirakana::job_execution_placement_policy_mode_label(windows_cpu_set_smt_worker_placement.selected_mode)
        << " windows_cpu_set_smt_worker_placement_topology_rows="
        << windows_cpu_set_smt_worker_placement.run_result.scheduling_evidence.scheduling_summary
               .worker_topology_row_count
        << " windows_cpu_set_smt_worker_placement_selected_cpu_sets="
        << windows_cpu_set_smt_worker_placement.placement_plan.selected_cpu_set_count
        << " windows_cpu_set_smt_worker_placement_distinct_cores="
        << windows_cpu_set_smt_worker_placement.placement_plan.distinct_core_count
        << " windows_cpu_set_smt_worker_placement_smt_sibling_cpu_sets="
        << windows_cpu_set_smt_worker_placement.placement_plan.smt_sibling_cpu_set_count
        << " windows_cpu_set_smt_worker_placement_smt_topology_known="
        << (windows_cpu_set_smt_worker_placement.placement_plan.smt_sibling_topology_known ? 1 : 0)
        << " windows_cpu_set_smt_worker_placement_smt_policy_applied="
        << (windows_cpu_set_smt_worker_placement.placement_plan.smt_sibling_policy_applied ? 1 : 0)
        << " windows_cpu_set_smt_worker_placement_worker_rows="
        << windows_cpu_set_smt_worker_placement.placement_plan.worker_rows.size()
        << " windows_cpu_set_smt_worker_placement_worker_threads_started="
        << windows_cpu_set_smt_worker_placement.run_result.worker_threads_started
        << " windows_cpu_set_smt_worker_placement_attempts="
        << windows_cpu_set_smt_worker_placement.run_result.worker_placement_attempt_count
        << " windows_cpu_set_smt_worker_placement_applied="
        << windows_cpu_set_smt_worker_placement.run_result.worker_placement_applied_count
        << " windows_cpu_set_smt_worker_placement_selected_cpu_set_applications="
        << windows_cpu_set_smt_worker_placement.run_result.worker_placement_selected_cpu_set_count
        << " windows_cpu_set_smt_worker_placement_tasks_submitted="
        << windows_cpu_set_smt_worker_placement.run_result.scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " windows_cpu_set_smt_worker_placement_tasks_executed="
        << windows_cpu_set_smt_worker_placement.run_result.tasks_executed
        << " windows_cpu_set_smt_worker_placement_tasks_failed="
        << windows_cpu_set_smt_worker_placement.run_result.tasks_failed
        << " windows_cpu_set_smt_worker_placement_task_side_effects="
        << windows_cpu_set_smt_worker_placement.task_side_effects
        << " windows_cpu_set_smt_worker_placement_native_thread_handles_exposed=0"
        << " windows_cpu_set_smt_worker_placement_hybrid_core_policy_applied=0"
        << " windows_cpu_set_smt_worker_placement_linux_affinity_applied=0"
        << " windows_cpu_set_smt_worker_placement_numa_allocation_applied=0"
        << " windows_cpu_set_smt_worker_placement_simd_dispatch_applied=0"
        << " windows_cpu_set_smt_worker_placement_gpu_async_overlap_applied=0"
        << " windows_cpu_set_smt_worker_placement_cuda_path_used=0"
        << " windows_cpu_set_smt_worker_placement_hip_path_used=0"
        << " windows_cpu_set_smt_worker_placement_sycl_path_used=0"
        << " simd_dispatch_policy_status=" << simd_dispatch_policy_status_name(simd_dispatch_policy)
        << " simd_dispatch_policy_ready=" << (simd_dispatch_policy_ready(simd_dispatch_policy) ? 1 : 0)
        << " simd_dispatch_policy_requested_lane="
        << mirakana::cpu_simd_lane_request_label(simd_dispatch_policy.dot_product.policy.requested_lane)
        << " simd_dispatch_policy_selected_lane="
        << mirakana::cpu_simd_lane_label(simd_dispatch_policy.dot_product.policy.selected_lane)
        << " simd_dispatch_policy_diagnostics=" << simd_dispatch_policy.dot_product.diagnostics.size()
        << " simd_dispatch_policy_input_count=" << simd_dispatch_policy.dot_product.input_count
        << " simd_dispatch_policy_dot_product_result=" << simd_dispatch_policy.dot_product.result
        << " simd_dispatch_policy_scalar_fallback=" << (simd_dispatch_policy.dot_product.policy.scalar_fallback ? 1 : 0)
        << " simd_dispatch_policy_sse2_compile_supported="
        << (simd_dispatch_policy.dot_product.policy.features.sse2_compile_supported ? 1 : 0)
        << " simd_dispatch_policy_sse2_runtime_supported="
        << (simd_dispatch_policy.dot_product.policy.features.sse2_runtime_supported ? 1 : 0)
        << " simd_dispatch_policy_sse2_selected=" << (simd_dispatch_policy.dot_product.policy.sse2_selected ? 1 : 0)
        << " simd_dispatch_policy_avx2_compile_supported="
        << (simd_dispatch_policy.dot_product.policy.features.avx2_compile_supported ? 1 : 0)
        << " simd_dispatch_policy_reviewed_avx2_target_available="
        << (simd_dispatch_policy.dot_product.policy.features.avx2_compile_supported ? 1 : 0)
        << " simd_dispatch_policy_avx2_runtime_supported="
        << (simd_dispatch_policy.dot_product.policy.features.avx2_runtime_supported ? 1 : 0)
        << " simd_dispatch_policy_avx2_selected=" << (simd_dispatch_policy.dot_product.policy.avx2_selected ? 1 : 0)
        << " simd_dispatch_policy_span_inputs_used=" << (simd_dispatch_policy.dot_product.span_inputs_used ? 1 : 0)
        << " simd_dispatch_policy_raw_pointers_retained="
        << (simd_dispatch_policy.dot_product.raw_pointers_retained ? 1 : 0)
        << " simd_dispatch_policy_native_handles_exposed=0"
        << " simd_dispatch_policy_numa_allocation_applied=0"
        << " simd_dispatch_policy_gpu_async_overlap_applied="
        << (simd_dispatch_policy.dot_product.policy.gpu_async_overlap_applied ? 1 : 0)
        << " simd_dispatch_policy_cuda_path_used=" << (simd_dispatch_policy.dot_product.policy.cuda_path_used ? 1 : 0)
        << " simd_dispatch_policy_hip_path_used=" << (simd_dispatch_policy.dot_product.policy.hip_path_used ? 1 : 0)
        << " simd_dispatch_policy_sycl_path_used=" << (simd_dispatch_policy.dot_product.policy.sycl_path_used ? 1 : 0)
        << " ui_overlay_requested=" << (report.native_ui_overlay_requested ? 1 : 0) << " ui_overlay_status="
        << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
        << " ui_overlay_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
        << " ui_overlay_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
        << " ui_overlay_draws=" << report.native_ui_overlay_draws
        << " ui_atlas_metadata_requested=" << (options.require_native_ui_textured_sprite_atlas ? 1 : 0)
        << " ui_atlas_metadata_status=" << ui_atlas_metadata_status_name(ui_atlas_metadata.status)
        << " ui_atlas_metadata_pages=" << ui_atlas_metadata.pages
        << " ui_atlas_metadata_bindings=" << ui_atlas_metadata.bindings
        << " ui_texture_overlay_requested=" << (report.native_ui_texture_overlay_requested ? 1 : 0)
        << " ui_texture_overlay_status="
        << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(
               report.native_ui_texture_overlay_status)
        << " ui_texture_overlay_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
        << " ui_texture_overlay_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
        << " ui_texture_overlay_texture_binds=" << report.native_ui_texture_overlay_texture_binds
        << " ui_texture_overlay_draws=" << report.native_ui_texture_overlay_draws
        << " framegraph_passes=" << report.framegraph_passes
        << " framegraph_passes_executed=" << report.renderer_stats.framegraph_passes_executed
        << " framegraph_render_passes_recorded=" << report.renderer_stats.framegraph_render_passes_recorded
        << " framegraph_barrier_steps_executed=" << report.renderer_stats.framegraph_barrier_steps_executed
        << " framegraph_multiqueue_status="
        << frame_graph_multi_queue_status_name(framegraph_multiqueue_requested, framegraph_multiqueue)
        << " d3d12_framegraph_multiqueue_evidence_ready="
        << (options.require_framegraph_multiqueue_evidence &&
                    d3d12_framegraph_multiqueue_evidence_ready(report, framegraph_multiqueue)
                ? 1
                : 0)
        << " d3d12_framegraph_multiqueue_evidence_selected="
        << (options.require_framegraph_multiqueue_evidence &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12
                ? 1
                : 0)
        << " vulkan_framegraph_multiqueue_evidence_ready="
        << (options.require_vulkan_framegraph_multiqueue_evidence &&
                    vulkan_framegraph_multiqueue_evidence_ready(report, framegraph_multiqueue)
                ? 1
                : 0)
        << " vulkan_framegraph_multiqueue_evidence_selected="
        << (options.require_vulkan_framegraph_multiqueue_evidence &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan
                ? 1
                : 0)
        << " framegraph_multiqueue_ready=" << (framegraph_multiqueue.ready ? 1 : 0)
        << " framegraph_multiqueue_diagnostics=" << framegraph_multiqueue.diagnostics.size()
        << " framegraph_multiqueue_command_lists_submitted=" << framegraph_multiqueue.command_lists_submitted
        << " framegraph_multiqueue_queue_waits_recorded=" << framegraph_multiqueue.queue_waits_recorded
        << " framegraph_multiqueue_barriers_recorded=" << framegraph_multiqueue.barriers_recorded
        << " framegraph_multiqueue_aliasing_barriers_recorded=" << framegraph_multiqueue.aliasing_barriers_recorded
        << " framegraph_multiqueue_pass_callbacks_invoked=" << framegraph_multiqueue.pass_callbacks_invoked
        << " framegraph_multiqueue_submitted_pass_fences=" << framegraph_multiqueue.submitted_pass_fences
        << " framegraph_multiqueue_copy_queue_submits=" << framegraph_multiqueue.copy_queue_submits
        << " framegraph_multiqueue_graphics_queue_submits=" << framegraph_multiqueue.graphics_queue_submits
        << " framegraph_multiqueue_queue_waits=" << framegraph_multiqueue.queue_waits
        << " framegraph_multiqueue_graphics_waited_for_copy="
        << (framegraph_multiqueue.graphics_waited_for_copy ? 1 : 0) << " renderer_quality_status="
        << mirakana::win32_desktop_presentation_quality_gate_status_name(renderer_quality.status)
        << " renderer_quality_ready=" << (renderer_quality.ready ? 1 : 0)
        << " renderer_quality_diagnostics=" << renderer_quality.diagnostics_count
        << " renderer_quality_expected_framegraph_passes=" << renderer_quality.expected_framegraph_passes
        << " renderer_quality_expected_framegraph_render_passes=" << renderer_quality.expected_framegraph_render_passes
        << " renderer_quality_expected_framegraph_barrier_steps=" << renderer_quality.expected_framegraph_barrier_steps
        << " renderer_quality_framegraph_passes_ok=" << (renderer_quality.framegraph_passes_current ? 1 : 0)
        << " renderer_quality_framegraph_render_passes_ok="
        << (renderer_quality.framegraph_render_passes_current ? 1 : 0)
        << " renderer_quality_framegraph_barrier_steps_ok="
        << (renderer_quality.framegraph_barrier_steps_current ? 1 : 0)
        << " renderer_quality_framegraph_execution_budget_ok="
        << (renderer_quality.framegraph_execution_budget_current ? 1 : 0)
        << " renderer_quality_scene_gpu_ready=" << (renderer_quality.scene_gpu_ready ? 1 : 0)
        << " renderer_quality_postprocess_ready=" << (renderer_quality.postprocess_ready ? 1 : 0)
        << " renderer_quality_postprocess_depth_input_ready="
        << (renderer_quality.postprocess_depth_input_ready ? 1 : 0)
        << " renderer_quality_directional_shadow_ready=" << (renderer_quality.directional_shadow_ready ? 1 : 0)
        << " renderer_quality_directional_shadow_filter_ready="
        << (renderer_quality.directional_shadow_filter_ready ? 1 : 0)
        << " renderer_gpu_skinning_draws=" << report.renderer_stats.gpu_skinning_draws
        << " renderer_skinned_palette_descriptor_binds=" << report.renderer_stats.skinned_palette_descriptor_binds
        << " d3d12_gpu_skinning_evidence_ready="
        << (options.require_d3d12_gpu_skinning_evidence && d3d12_gpu_skinning_evidence_ready(report, options.max_frames)
                ? 1
                : 0)
        << " d3d12_gpu_skinning_evidence_selected="
        << (options.require_d3d12_gpu_skinning_evidence &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12
                ? 1
                : 0)
        << " vulkan_gpu_skinning_evidence_ready="
        << (options.require_vulkan_gpu_skinning_evidence &&
                    vulkan_gpu_skinning_evidence_ready(report, options.max_frames)
                ? 1
                : 0)
        << " vulkan_gpu_skinning_evidence_selected="
        << (options.require_vulkan_gpu_skinning_evidence &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan
                ? 1
                : 0);
    if (environment_ready_aggregate.requested) {
        std::cout << " environment_ready_status="
                  << environment_ready_aggregate_status_name(environment_ready_aggregate.status)
                  << " environment_ready=" << (environment_ready_aggregate.ready ? 1 : 0)
                  << " environment_ready_profile_v2=" << (environment_ready_aggregate.profile_v2_ready ? 1 : 0)
                  << " environment_ready_d3d12_primary=" << (environment_ready_aggregate.d3d12_primary_ready ? 1 : 0)
                  << " environment_ready_vulkan_strict=" << (environment_ready_aggregate.vulkan_strict_ready ? 1 : 0)
                  << " environment_ready_metal_host=" << (environment_ready_aggregate.metal_host_ready ? 1 : 0)
                  << " environment_ready_backend_parity=" << (environment_ready_aggregate.backend_parity_ready ? 1 : 0)
                  << " environment_ready_broad_optimization_claimed="
                  << (environment_ready_aggregate.broad_optimization_claimed ? 1 : 0)
                  << " environment_ready_native_handle_access="
                  << (environment_ready_aggregate.native_handle_access ? 1 : 0)
                  << " environment_ready_diagnostics=" << environment_ready_aggregate.diagnostics;
    }
    if (environment_vulkan_strict_aggregate.requested) {
        std::cout
            << " environment_vulkan_strict_aggregate_status="
            << environment_ready_aggregate_status_name(environment_vulkan_strict_aggregate.status)
            << " environment_vulkan_strict_aggregate_ready=" << (environment_vulkan_strict_aggregate.ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_profile_v2="
            << (environment_vulkan_strict_aggregate.profile_v2_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_selected_backend="
            << (environment_vulkan_strict_aggregate.vulkan_backend_selected ? "vulkan" : "not_vulkan")
            << " environment_vulkan_strict_aggregate_postprocess="
            << (environment_vulkan_strict_aggregate.postprocess_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_fog=" << (environment_vulkan_strict_aggregate.fog_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_physical_sky="
            << (environment_vulkan_strict_aggregate.physical_sky_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_lighting="
            << (environment_vulkan_strict_aggregate.lighting_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_volumetric_fog="
            << (environment_vulkan_strict_aggregate.volumetric_fog_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_volumetric_cloud="
            << (environment_vulkan_strict_aggregate.volumetric_cloud_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_precipitation="
            << (environment_vulkan_strict_aggregate.precipitation_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_quality_budget="
            << (environment_vulkan_strict_aggregate.quality_budget_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_feature_rows=" << environment_vulkan_strict_aggregate.feature_rows
            << " environment_vulkan_strict_aggregate_descriptor_set_bindings="
            << environment_vulkan_strict_aggregate.descriptor_set_bindings
            << " environment_vulkan_strict_aggregate_toolchain_ready="
            << (environment_vulkan_strict_aggregate.toolchain_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_vulkan_sdk_tools_ready="
            << (environment_vulkan_strict_aggregate.vulkan_sdk_tools_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_dxc_spirv_codegen_ready="
            << (environment_vulkan_strict_aggregate.dxc_spirv_codegen_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_spirv_validation_ready="
            << (environment_vulkan_strict_aggregate.spirv_validation_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_validation_layers_ready="
            << (environment_vulkan_strict_aggregate.validation_layers_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_device_features_ready="
            << (environment_vulkan_strict_aggregate.device_features_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_toolchain_rows="
            << environment_vulkan_strict_aggregate.toolchain_rows
            << " environment_vulkan_strict_aggregate_missing_toolchain_rows="
            << environment_vulkan_strict_aggregate.missing_toolchain_rows
            << " environment_vulkan_strict_aggregate_missing_validation_layer_rows="
            << environment_vulkan_strict_aggregate.missing_validation_layer_rows
            << " environment_vulkan_strict_aggregate_missing_spirv_validation_rows="
            << environment_vulkan_strict_aggregate.missing_spirv_validation_rows
            << " environment_vulkan_strict_aggregate_unsupported_feature_device_rows="
            << environment_vulkan_strict_aggregate.unsupported_feature_device_rows
            << " environment_vulkan_strict_aggregate_synchronization2_barriers="
            << environment_vulkan_strict_aggregate.synchronization2_barriers
            << " environment_vulkan_strict_aggregate_resource_usage_layout_ready="
            << (environment_vulkan_strict_aggregate.resource_usage_layout_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_resource_usage_layout_rows="
            << environment_vulkan_strict_aggregate.resource_usage_layout_rows
            << " environment_vulkan_strict_aggregate_attachment_usage_layout_rows="
            << environment_vulkan_strict_aggregate.attachment_usage_layout_rows
            << " environment_vulkan_strict_aggregate_sampled_texture_usage_layout_rows="
            << environment_vulkan_strict_aggregate.sampled_texture_usage_layout_rows
            << " environment_vulkan_strict_aggregate_storage_buffer_usage_layout_rows="
            << environment_vulkan_strict_aggregate.storage_buffer_usage_layout_rows
            << " environment_vulkan_strict_aggregate_cube_map_usage_layout_rows="
            << environment_vulkan_strict_aggregate.cube_map_usage_layout_rows
            << " environment_vulkan_strict_aggregate_weather_texture_usage_layout_rows="
            << environment_vulkan_strict_aggregate.weather_texture_usage_layout_rows
            << " environment_vulkan_strict_aggregate_froxel_buffer_usage_layout_rows="
            << environment_vulkan_strict_aggregate.froxel_buffer_usage_layout_rows
            << " environment_vulkan_strict_aggregate_readback_resource_usage_layout_rows="
            << environment_vulkan_strict_aggregate.readback_resource_usage_layout_rows
            << " environment_vulkan_strict_aggregate_renderer_draws="
            << environment_vulkan_strict_aggregate.renderer_draws
            << " environment_vulkan_strict_aggregate_compute_dispatches="
            << environment_vulkan_strict_aggregate.compute_dispatches
            << " environment_vulkan_strict_aggregate_texture_uploads="
            << environment_vulkan_strict_aggregate.texture_uploads
            << " environment_vulkan_strict_aggregate_readback_rows="
            << environment_vulkan_strict_aggregate.readback_rows
            << " environment_vulkan_strict_aggregate_native_handle_access="
            << (environment_vulkan_strict_aggregate.native_handle_access ? 1 : 0)
            << " environment_vulkan_strict_aggregate_d3d12_fallback="
            << (environment_vulkan_strict_aggregate.d3d12_fallback ? 1 : 0)
            << " environment_vulkan_strict_aggregate_metal_fallback="
            << (environment_vulkan_strict_aggregate.metal_fallback ? 1 : 0)
            << " environment_vulkan_strict_aggregate_backend_parity="
            << (environment_vulkan_strict_aggregate.backend_parity_ready ? 1 : 0)
            << " environment_vulkan_strict_aggregate_broad_optimization_claimed="
            << (environment_vulkan_strict_aggregate.broad_optimization_claimed ? 1 : 0)
            << " environment_vulkan_strict_aggregate_diagnostics=" << environment_vulkan_strict_aggregate.diagnostics;
    }
    if (environment_backend_parity.requested) {
        const auto& plan = environment_backend_parity.plan;
        std::cout << " environment_backend_parity_status=" << environment_backend_parity_status_name(plan.status)
                  << " environment_backend_parity_ready=" << (plan.environment_backend_parity_ready ? 1 : 0)
                  << " environment_backend_parity_required_backends=" << plan.required_backends.size()
                  << " environment_backend_parity_required_features=" << plan.required_feature_count
                  << " environment_backend_parity_rows=" << plan.row_count
                  << " environment_backend_parity_ready_rows=" << plan.ready_row_count
                  << " environment_backend_parity_host_gated_rows=" << plan.host_gated_row_count
                  << " environment_backend_parity_host_validated_backends=" << plan.host_validated_backend_count
                  << " environment_backend_parity_d3d12_primary=" << (plan.d3d12_primary_ready ? 1 : 0)
                  << " environment_backend_parity_vulkan_strict=" << (plan.vulkan_strict_ready ? 1 : 0)
                  << " environment_backend_parity_metal_host=" << (plan.metal_host_ready ? 1 : 0)
                  << " environment_backend_parity_requires_metal_host_evidence="
                  << (plan.requires_metal_host_evidence ? 1 : 0)
                  << " environment_backend_parity_diagnostics=" << plan.diagnostics.size()
                  << " environment_backend_parity_native_handle_access=" << (plan.exposed_native_handles ? 1 : 0)
                  << " environment_backend_parity_invoked_gpu_commands=" << (plan.invoked_gpu_commands ? 1 : 0)
                  << " environment_backend_parity_all_platform_ready=0"
                  << " environment_backend_parity_commercial_ready=0"
                  << " environment_backend_parity_broad_environment_ready=0"
                  << " environment_backend_parity_broad_optimization_ready=0"
                  << " environment_backend_parity_replay_hash=" << plan.replay_hash;
    }
    if (environment_platform_readiness.requested) {
        std::cout
            << " environment_platform_readiness_status=host_evidence_required"
            << " environment_platform_readiness_ready=" << (environment_platform_readiness.ready ? 1 : 0)
            << " environment_platform_readiness_rows=" << environment_platform_readiness.rows
            << " environment_platform_readiness_ready_rows=" << environment_platform_readiness.ready_rows
            << " environment_platform_readiness_host_gated_rows=" << environment_platform_readiness.host_gated_rows
            << " environment_platform_windows_d3d12_ready="
            << (environment_platform_readiness.windows_d3d12_ready ? 1 : 0)
            << " environment_platform_windows_vulkan_ready="
            << (environment_platform_readiness.windows_vulkan_ready ? 1 : 0)
            << " environment_platform_linux_vulkan_ready="
            << (environment_platform_readiness.linux_vulkan_ready ? 1 : 0)
            << " environment_platform_macos_metal_ready=" << (environment_platform_readiness.macos_metal_ready ? 1 : 0)
            << " environment_platform_ios_metal_ready=" << (environment_platform_readiness.ios_metal_ready ? 1 : 0)
            << " environment_platform_android_vulkan_ready="
            << (environment_platform_readiness.android_vulkan_ready ? 1 : 0)
            << " environment_platform_requires_windows_vulkan_host_evidence="
            << (environment_platform_readiness.requires_windows_vulkan_host_evidence ? 1 : 0)
            << " environment_platform_requires_linux_vulkan_host_evidence="
            << (environment_platform_readiness.requires_linux_vulkan_host_evidence ? 1 : 0)
            << " environment_platform_requires_macos_metal_host_evidence="
            << (environment_platform_readiness.requires_macos_metal_host_evidence ? 1 : 0)
            << " environment_platform_requires_ios_metal_host_evidence="
            << (environment_platform_readiness.requires_ios_metal_host_evidence ? 1 : 0)
            << " environment_platform_requires_android_vulkan_host_evidence="
            << (environment_platform_readiness.requires_android_vulkan_host_evidence ? 1 : 0)
            << " environment_all_platform_unconditional_ready=0"
            << " environment_platform_backend_parity_ready="
            << (environment_platform_readiness.backend_parity_ready ? 1 : 0)
            << " environment_platform_commercial_ready=" << (environment_platform_readiness.commercial_ready ? 1 : 0)
            << " environment_platform_broad_environment_ready="
            << (environment_platform_readiness.broad_environment_ready ? 1 : 0)
            << " environment_platform_broad_optimization_ready="
            << (environment_platform_readiness.broad_optimization_ready ? 1 : 0)
            << " environment_platform_native_handle_access="
            << (environment_platform_readiness.native_handle_access ? 1 : 0)
            << " environment_platform_invoked_gpu_commands="
            << (environment_platform_readiness.invoked_gpu_commands ? 1 : 0)
            << " environment_platform_diagnostics=" << environment_platform_readiness.diagnostics
            << " environment_platform_readiness_replay_hash=" << environment_platform_readiness.replay_hash;
    }
    if (environment_optimization_measurement.requested) {
        const auto& plan = environment_optimization_measurement.plan;
        const auto* preset_row =
            find_environment_optimization_row(plan, mirakana::EnvironmentOptimizationWorkload::preset_pack_flythrough);
        const auto* storm_row =
            find_environment_optimization_row(plan, mirakana::EnvironmentOptimizationWorkload::storm_precipitation);
        const auto* dense_fog_row =
            find_environment_optimization_row(plan, mirakana::EnvironmentOptimizationWorkload::dense_volumetric_fog);
        const auto* cloud_sunset_row =
            find_environment_optimization_row(plan, mirakana::EnvironmentOptimizationWorkload::volumetric_cloud_sunset);
        const auto* snowfield_row = find_environment_optimization_row(
            plan, mirakana::EnvironmentOptimizationWorkload::snowfield_material_weathering);
        const auto* weather_stress_row = find_environment_optimization_row(
            plan, mirakana::EnvironmentOptimizationWorkload::weather_simulation_stress);
        const auto* asset_cold_load_row =
            find_environment_optimization_row(plan, mirakana::EnvironmentOptimizationWorkload::asset_library_cold_load);
        std::cout << " environment_optimization_measurement_status="
                  << environment_optimization_measurement_status_name(plan.status)
                  << " environment_optimization_measurement_ready="
                  << (plan.environment_broad_optimization_ready ? 1 : 0)
                  << " environment_optimization_measurement_workload_rows=" << plan.row_count
                  << " environment_optimization_measurement_required_workloads=" << plan.required_workload_count
                  << " environment_optimization_measurement_measured_workloads=" << plan.measured_workload_count
                  << " environment_optimization_measurement_before_after_pairs=" << plan.before_after_pair_count
                  << " environment_optimization_measurement_backend=d3d12"
                  << " environment_optimization_measurement_profile=preset_pack_flythrough"
                  << " environment_optimization_measurement_profiles=preset_pack_flythrough,storm_precipitation,dense_"
                     "volumetric_fog,volumetric_cloud_sunset,snowfield_material_weathering,weather_simulation_stress,"
                     "asset_library_cold_load";
        if (preset_row != nullptr) {
            const auto& row = *preset_row;
            std::cout << " environment_optimization_preset_pack_flythrough_ready="
                      << (plan.d3d12_preset_pack_flythrough_measured ? 1 : 0)
                      << " environment_optimization_measurement_warmup_frames=" << row.warmup_frames
                      << " environment_optimization_measurement_sample_frames=" << row.sample_frames
                      << " environment_optimization_measurement_cpu_frame_p95_before_us=" << row.before.cpu_frame_p95_us
                      << " environment_optimization_measurement_cpu_frame_p95_after_us=" << row.after.cpu_frame_p95_us
                      << " environment_optimization_measurement_gpu_frame_p95_before_us=" << row.before.gpu_frame_p95_us
                      << " environment_optimization_measurement_gpu_frame_p95_after_us=" << row.after.gpu_frame_p95_us
                      << " environment_optimization_measurement_memory_peak_before_bytes="
                      << row.before.memory_peak_bytes
                      << " environment_optimization_measurement_memory_peak_after_bytes=" << row.after.memory_peak_bytes
                      << " environment_optimization_measurement_transient_gpu_before_bytes="
                      << row.before.transient_gpu_bytes
                      << " environment_optimization_measurement_transient_gpu_after_bytes="
                      << row.after.transient_gpu_bytes
                      << " environment_optimization_measurement_upload_before_bytes=" << row.before.upload_bytes
                      << " environment_optimization_measurement_upload_after_bytes=" << row.after.upload_bytes
                      << " environment_optimization_measurement_draw_count_before=" << row.before.draw_count
                      << " environment_optimization_measurement_draw_count_after=" << row.after.draw_count
                      << " environment_optimization_measurement_dispatch_count_before=" << row.before.dispatch_count
                      << " environment_optimization_measurement_dispatch_count_after=" << row.after.dispatch_count
                      << " environment_optimization_measurement_barrier_count_before=" << row.before.barrier_count
                      << " environment_optimization_measurement_barrier_count_after=" << row.after.barrier_count
                      << " environment_optimization_measurement_texture_residency_before_bytes="
                      << row.before.texture_residency_bytes
                      << " environment_optimization_measurement_texture_residency_after_bytes="
                      << row.after.texture_residency_bytes
                      << " environment_optimization_measurement_package_load_before_us=" << row.before.package_load_us
                      << " environment_optimization_measurement_package_load_after_us=" << row.after.package_load_us
                      << " environment_optimization_measurement_stutter_frames_before=" << row.before.stutter_frames
                      << " environment_optimization_measurement_stutter_frames_after=" << row.after.stutter_frames;
        }
        if (storm_row != nullptr) {
            const auto& row = *storm_row;
            std::cout
                << " environment_optimization_storm_precipitation_ready="
                << (plan.d3d12_storm_precipitation_measured ? 1 : 0)
                << " environment_optimization_storm_precipitation_warmup_frames=" << row.warmup_frames
                << " environment_optimization_storm_precipitation_sample_frames=" << row.sample_frames
                << " environment_optimization_storm_precipitation_cpu_frame_p95_before_us="
                << row.before.cpu_frame_p95_us
                << " environment_optimization_storm_precipitation_cpu_frame_p95_after_us=" << row.after.cpu_frame_p95_us
                << " environment_optimization_storm_precipitation_gpu_frame_p95_before_us="
                << row.before.gpu_frame_p95_us
                << " environment_optimization_storm_precipitation_gpu_frame_p95_after_us=" << row.after.gpu_frame_p95_us
                << " environment_optimization_storm_precipitation_memory_peak_before_bytes="
                << row.before.memory_peak_bytes
                << " environment_optimization_storm_precipitation_memory_peak_after_bytes="
                << row.after.memory_peak_bytes
                << " environment_optimization_storm_precipitation_transient_gpu_before_bytes="
                << row.before.transient_gpu_bytes
                << " environment_optimization_storm_precipitation_transient_gpu_after_bytes="
                << row.after.transient_gpu_bytes
                << " environment_optimization_storm_precipitation_upload_before_bytes=" << row.before.upload_bytes
                << " environment_optimization_storm_precipitation_upload_after_bytes=" << row.after.upload_bytes
                << " environment_optimization_storm_precipitation_draw_count_before=" << row.before.draw_count
                << " environment_optimization_storm_precipitation_draw_count_after=" << row.after.draw_count
                << " environment_optimization_storm_precipitation_dispatch_count_before=" << row.before.dispatch_count
                << " environment_optimization_storm_precipitation_dispatch_count_after=" << row.after.dispatch_count
                << " environment_optimization_storm_precipitation_barrier_count_before=" << row.before.barrier_count
                << " environment_optimization_storm_precipitation_barrier_count_after=" << row.after.barrier_count
                << " environment_optimization_storm_precipitation_texture_residency_before_bytes="
                << row.before.texture_residency_bytes
                << " environment_optimization_storm_precipitation_texture_residency_after_bytes="
                << row.after.texture_residency_bytes
                << " environment_optimization_storm_precipitation_package_load_before_us=" << row.before.package_load_us
                << " environment_optimization_storm_precipitation_package_load_after_us=" << row.after.package_load_us
                << " environment_optimization_storm_precipitation_stutter_frames_before=" << row.before.stutter_frames
                << " environment_optimization_storm_precipitation_stutter_frames_after=" << row.after.stutter_frames;
        }
        if (dense_fog_row != nullptr) {
            const auto& row = *dense_fog_row;
            std::cout
                << " environment_optimization_dense_volumetric_fog_ready="
                << (plan.d3d12_dense_volumetric_fog_measured ? 1 : 0)
                << " environment_optimization_dense_volumetric_fog_warmup_frames=" << row.warmup_frames
                << " environment_optimization_dense_volumetric_fog_sample_frames=" << row.sample_frames
                << " environment_optimization_dense_volumetric_fog_cpu_frame_p95_before_us="
                << row.before.cpu_frame_p95_us
                << " environment_optimization_dense_volumetric_fog_cpu_frame_p95_after_us="
                << row.after.cpu_frame_p95_us
                << " environment_optimization_dense_volumetric_fog_gpu_frame_p95_before_us="
                << row.before.gpu_frame_p95_us
                << " environment_optimization_dense_volumetric_fog_gpu_frame_p95_after_us="
                << row.after.gpu_frame_p95_us
                << " environment_optimization_dense_volumetric_fog_memory_peak_before_bytes="
                << row.before.memory_peak_bytes
                << " environment_optimization_dense_volumetric_fog_memory_peak_after_bytes="
                << row.after.memory_peak_bytes
                << " environment_optimization_dense_volumetric_fog_transient_gpu_before_bytes="
                << row.before.transient_gpu_bytes
                << " environment_optimization_dense_volumetric_fog_transient_gpu_after_bytes="
                << row.after.transient_gpu_bytes
                << " environment_optimization_dense_volumetric_fog_upload_before_bytes=" << row.before.upload_bytes
                << " environment_optimization_dense_volumetric_fog_upload_after_bytes=" << row.after.upload_bytes
                << " environment_optimization_dense_volumetric_fog_draw_count_before=" << row.before.draw_count
                << " environment_optimization_dense_volumetric_fog_draw_count_after=" << row.after.draw_count
                << " environment_optimization_dense_volumetric_fog_dispatch_count_before=" << row.before.dispatch_count
                << " environment_optimization_dense_volumetric_fog_dispatch_count_after=" << row.after.dispatch_count
                << " environment_optimization_dense_volumetric_fog_barrier_count_before=" << row.before.barrier_count
                << " environment_optimization_dense_volumetric_fog_barrier_count_after=" << row.after.barrier_count
                << " environment_optimization_dense_volumetric_fog_texture_residency_before_bytes="
                << row.before.texture_residency_bytes
                << " environment_optimization_dense_volumetric_fog_texture_residency_after_bytes="
                << row.after.texture_residency_bytes
                << " environment_optimization_dense_volumetric_fog_package_load_before_us="
                << row.before.package_load_us
                << " environment_optimization_dense_volumetric_fog_package_load_after_us=" << row.after.package_load_us
                << " environment_optimization_dense_volumetric_fog_stutter_frames_before=" << row.before.stutter_frames
                << " environment_optimization_dense_volumetric_fog_stutter_frames_after=" << row.after.stutter_frames;
        }
        if (cloud_sunset_row != nullptr) {
            const auto& row = *cloud_sunset_row;
            std::cout
                << " environment_optimization_volumetric_cloud_sunset_ready="
                << (plan.d3d12_volumetric_cloud_sunset_measured ? 1 : 0)
                << " environment_optimization_volumetric_cloud_sunset_warmup_frames=" << row.warmup_frames
                << " environment_optimization_volumetric_cloud_sunset_sample_frames=" << row.sample_frames
                << " environment_optimization_volumetric_cloud_sunset_cpu_frame_p95_before_us="
                << row.before.cpu_frame_p95_us
                << " environment_optimization_volumetric_cloud_sunset_cpu_frame_p95_after_us="
                << row.after.cpu_frame_p95_us
                << " environment_optimization_volumetric_cloud_sunset_gpu_frame_p95_before_us="
                << row.before.gpu_frame_p95_us
                << " environment_optimization_volumetric_cloud_sunset_gpu_frame_p95_after_us="
                << row.after.gpu_frame_p95_us
                << " environment_optimization_volumetric_cloud_sunset_memory_peak_before_bytes="
                << row.before.memory_peak_bytes
                << " environment_optimization_volumetric_cloud_sunset_memory_peak_after_bytes="
                << row.after.memory_peak_bytes
                << " environment_optimization_volumetric_cloud_sunset_transient_gpu_before_bytes="
                << row.before.transient_gpu_bytes
                << " environment_optimization_volumetric_cloud_sunset_transient_gpu_after_bytes="
                << row.after.transient_gpu_bytes
                << " environment_optimization_volumetric_cloud_sunset_upload_before_bytes=" << row.before.upload_bytes
                << " environment_optimization_volumetric_cloud_sunset_upload_after_bytes=" << row.after.upload_bytes
                << " environment_optimization_volumetric_cloud_sunset_draw_count_before=" << row.before.draw_count
                << " environment_optimization_volumetric_cloud_sunset_draw_count_after=" << row.after.draw_count
                << " environment_optimization_volumetric_cloud_sunset_dispatch_count_before="
                << row.before.dispatch_count
                << " environment_optimization_volumetric_cloud_sunset_dispatch_count_after=" << row.after.dispatch_count
                << " environment_optimization_volumetric_cloud_sunset_barrier_count_before=" << row.before.barrier_count
                << " environment_optimization_volumetric_cloud_sunset_barrier_count_after=" << row.after.barrier_count
                << " environment_optimization_volumetric_cloud_sunset_texture_residency_before_bytes="
                << row.before.texture_residency_bytes
                << " environment_optimization_volumetric_cloud_sunset_texture_residency_after_bytes="
                << row.after.texture_residency_bytes
                << " environment_optimization_volumetric_cloud_sunset_package_load_before_us="
                << row.before.package_load_us
                << " environment_optimization_volumetric_cloud_sunset_package_load_after_us="
                << row.after.package_load_us
                << " environment_optimization_volumetric_cloud_sunset_stutter_frames_before="
                << row.before.stutter_frames
                << " environment_optimization_volumetric_cloud_sunset_stutter_frames_after="
                << row.after.stutter_frames;
        }
        if (snowfield_row != nullptr) {
            const auto& row = *snowfield_row;
            std::cout << " environment_optimization_snowfield_material_weathering_ready="
                      << (plan.d3d12_snowfield_material_weathering_measured ? 1 : 0)
                      << " environment_optimization_snowfield_material_weathering_warmup_frames=" << row.warmup_frames
                      << " environment_optimization_snowfield_material_weathering_sample_frames=" << row.sample_frames
                      << " environment_optimization_snowfield_material_weathering_cpu_frame_p95_before_us="
                      << row.before.cpu_frame_p95_us
                      << " environment_optimization_snowfield_material_weathering_cpu_frame_p95_after_us="
                      << row.after.cpu_frame_p95_us
                      << " environment_optimization_snowfield_material_weathering_gpu_frame_p95_before_us="
                      << row.before.gpu_frame_p95_us
                      << " environment_optimization_snowfield_material_weathering_gpu_frame_p95_after_us="
                      << row.after.gpu_frame_p95_us
                      << " environment_optimization_snowfield_material_weathering_memory_peak_before_bytes="
                      << row.before.memory_peak_bytes
                      << " environment_optimization_snowfield_material_weathering_memory_peak_after_bytes="
                      << row.after.memory_peak_bytes
                      << " environment_optimization_snowfield_material_weathering_transient_gpu_before_bytes="
                      << row.before.transient_gpu_bytes
                      << " environment_optimization_snowfield_material_weathering_transient_gpu_after_bytes="
                      << row.after.transient_gpu_bytes
                      << " environment_optimization_snowfield_material_weathering_upload_before_bytes="
                      << row.before.upload_bytes
                      << " environment_optimization_snowfield_material_weathering_upload_after_bytes="
                      << row.after.upload_bytes
                      << " environment_optimization_snowfield_material_weathering_draw_count_before="
                      << row.before.draw_count
                      << " environment_optimization_snowfield_material_weathering_draw_count_after="
                      << row.after.draw_count
                      << " environment_optimization_snowfield_material_weathering_dispatch_count_before="
                      << row.before.dispatch_count
                      << " environment_optimization_snowfield_material_weathering_dispatch_count_after="
                      << row.after.dispatch_count
                      << " environment_optimization_snowfield_material_weathering_barrier_count_before="
                      << row.before.barrier_count
                      << " environment_optimization_snowfield_material_weathering_barrier_count_after="
                      << row.after.barrier_count
                      << " environment_optimization_snowfield_material_weathering_texture_residency_before_bytes="
                      << row.before.texture_residency_bytes
                      << " environment_optimization_snowfield_material_weathering_texture_residency_after_bytes="
                      << row.after.texture_residency_bytes
                      << " environment_optimization_snowfield_material_weathering_package_load_before_us="
                      << row.before.package_load_us
                      << " environment_optimization_snowfield_material_weathering_package_load_after_us="
                      << row.after.package_load_us
                      << " environment_optimization_snowfield_material_weathering_stutter_frames_before="
                      << row.before.stutter_frames
                      << " environment_optimization_snowfield_material_weathering_stutter_frames_after="
                      << row.after.stutter_frames;
        }
        if (weather_stress_row != nullptr) {
            const auto& row = *weather_stress_row;
            std::cout
                << " environment_optimization_weather_simulation_stress_ready="
                << (plan.d3d12_weather_simulation_stress_measured ? 1 : 0)
                << " environment_optimization_weather_simulation_stress_warmup_frames=" << row.warmup_frames
                << " environment_optimization_weather_simulation_stress_sample_frames=" << row.sample_frames
                << " environment_optimization_weather_simulation_stress_cpu_frame_p95_before_us="
                << row.before.cpu_frame_p95_us
                << " environment_optimization_weather_simulation_stress_cpu_frame_p95_after_us="
                << row.after.cpu_frame_p95_us
                << " environment_optimization_weather_simulation_stress_gpu_frame_p95_before_us="
                << row.before.gpu_frame_p95_us
                << " environment_optimization_weather_simulation_stress_gpu_frame_p95_after_us="
                << row.after.gpu_frame_p95_us
                << " environment_optimization_weather_simulation_stress_memory_peak_before_bytes="
                << row.before.memory_peak_bytes
                << " environment_optimization_weather_simulation_stress_memory_peak_after_bytes="
                << row.after.memory_peak_bytes
                << " environment_optimization_weather_simulation_stress_transient_gpu_before_bytes="
                << row.before.transient_gpu_bytes
                << " environment_optimization_weather_simulation_stress_transient_gpu_after_bytes="
                << row.after.transient_gpu_bytes
                << " environment_optimization_weather_simulation_stress_upload_before_bytes=" << row.before.upload_bytes
                << " environment_optimization_weather_simulation_stress_upload_after_bytes=" << row.after.upload_bytes
                << " environment_optimization_weather_simulation_stress_draw_count_before=" << row.before.draw_count
                << " environment_optimization_weather_simulation_stress_draw_count_after=" << row.after.draw_count
                << " environment_optimization_weather_simulation_stress_dispatch_count_before="
                << row.before.dispatch_count
                << " environment_optimization_weather_simulation_stress_dispatch_count_after="
                << row.after.dispatch_count
                << " environment_optimization_weather_simulation_stress_barrier_count_before="
                << row.before.barrier_count
                << " environment_optimization_weather_simulation_stress_barrier_count_after=" << row.after.barrier_count
                << " environment_optimization_weather_simulation_stress_texture_residency_before_bytes="
                << row.before.texture_residency_bytes
                << " environment_optimization_weather_simulation_stress_texture_residency_after_bytes="
                << row.after.texture_residency_bytes
                << " environment_optimization_weather_simulation_stress_package_load_before_us="
                << row.before.package_load_us
                << " environment_optimization_weather_simulation_stress_package_load_after_us="
                << row.after.package_load_us
                << " environment_optimization_weather_simulation_stress_stutter_frames_before="
                << row.before.stutter_frames
                << " environment_optimization_weather_simulation_stress_stutter_frames_after="
                << row.after.stutter_frames;
        }
        if (asset_cold_load_row != nullptr) {
            const auto& row = *asset_cold_load_row;
            std::cout
                << " environment_optimization_asset_library_cold_load_ready="
                << (plan.d3d12_asset_library_cold_load_measured ? 1 : 0)
                << " environment_optimization_asset_library_cold_load_warmup_frames=" << row.warmup_frames
                << " environment_optimization_asset_library_cold_load_sample_frames=" << row.sample_frames
                << " environment_optimization_asset_library_cold_load_cpu_frame_p95_before_us="
                << row.before.cpu_frame_p95_us
                << " environment_optimization_asset_library_cold_load_cpu_frame_p95_after_us="
                << row.after.cpu_frame_p95_us
                << " environment_optimization_asset_library_cold_load_gpu_frame_p95_before_us="
                << row.before.gpu_frame_p95_us
                << " environment_optimization_asset_library_cold_load_gpu_frame_p95_after_us="
                << row.after.gpu_frame_p95_us
                << " environment_optimization_asset_library_cold_load_memory_peak_before_bytes="
                << row.before.memory_peak_bytes
                << " environment_optimization_asset_library_cold_load_memory_peak_after_bytes="
                << row.after.memory_peak_bytes
                << " environment_optimization_asset_library_cold_load_transient_gpu_before_bytes="
                << row.before.transient_gpu_bytes
                << " environment_optimization_asset_library_cold_load_transient_gpu_after_bytes="
                << row.after.transient_gpu_bytes
                << " environment_optimization_asset_library_cold_load_upload_before_bytes=" << row.before.upload_bytes
                << " environment_optimization_asset_library_cold_load_upload_after_bytes=" << row.after.upload_bytes
                << " environment_optimization_asset_library_cold_load_draw_count_before=" << row.before.draw_count
                << " environment_optimization_asset_library_cold_load_draw_count_after=" << row.after.draw_count
                << " environment_optimization_asset_library_cold_load_dispatch_count_before="
                << row.before.dispatch_count
                << " environment_optimization_asset_library_cold_load_dispatch_count_after=" << row.after.dispatch_count
                << " environment_optimization_asset_library_cold_load_barrier_count_before=" << row.before.barrier_count
                << " environment_optimization_asset_library_cold_load_barrier_count_after=" << row.after.barrier_count
                << " environment_optimization_asset_library_cold_load_texture_residency_before_bytes="
                << row.before.texture_residency_bytes
                << " environment_optimization_asset_library_cold_load_texture_residency_after_bytes="
                << row.after.texture_residency_bytes
                << " environment_optimization_asset_library_cold_load_package_load_before_us="
                << row.before.package_load_us
                << " environment_optimization_asset_library_cold_load_package_load_after_us="
                << row.after.package_load_us
                << " environment_optimization_asset_library_cold_load_stutter_frames_before="
                << row.before.stutter_frames
                << " environment_optimization_asset_library_cold_load_stutter_frames_after="
                << row.after.stutter_frames;
        }
        std::cout << " environment_optimization_measurement_regression_budget_rows=" << plan.regression_budget_row_count
                  << " environment_optimization_measurement_over_budget=" << plan.over_budget_row_count
                  << " environment_optimization_measurement_backend_parity_ready="
                  << (plan.environment_backend_parity_ready ? 1 : 0)
                  << " environment_broad_optimization_ready=" << (plan.environment_broad_optimization_ready ? 1 : 0)
                  << " environment_optimization_measurement_native_handle_access="
                  << (plan.exposed_native_handles ? 1 : 0)
                  << " environment_optimization_measurement_invoked_gpu_commands="
                  << (plan.invoked_gpu_commands ? 1 : 0)
                  << " environment_optimization_measurement_diagnostics=" << plan.diagnostics.size()
                  << " environment_optimization_measurement_replay_hash=" << plan.replay_hash;
    }
    if (environment_weather_simulation_package.requested) {
        const auto& plan = environment_weather_simulation_package.plan;
        std::cout << " environment_weather_simulation_package_status="
                  << environment_weather_simulation_package_status_name(plan)
                  << " environment_weather_simulation_package_ready=" << (plan.succeeded() ? 1 : 0)
                  << " environment_weather_simulation_steps=" << environment_weather_simulation_package.step_count
                  << " environment_weather_simulation_cells=" << plan.cell_count
                  << " environment_weather_simulation_effective_timestep_ms="
                  << seconds_to_milliseconds(plan.effective_timestep_s)
                  << " environment_weather_simulation_timestep_clamped=" << (plan.timestep_clamped ? 1 : 0)
                  << " environment_weather_simulation_total_water_before_mg="
                  << kilograms_to_milligrams(plan.total_water_before_kg)
                  << " environment_weather_simulation_total_water_after_mg="
                  << kilograms_to_milligrams(plan.total_water_after_kg)
                  << " environment_weather_simulation_water_conservation_error_mg="
                  << kilograms_to_milligrams(plan.water_conservation_error_kg)
                  << " environment_weather_simulation_water_conservation_error_bound_mg="
                  << environment_weather_simulation_package_water_error_bound_mg()
                  << " environment_weather_simulation_max_cell_water_conservation_error_mg_per_m2="
                  << kilograms_to_milligrams(plan.max_cell_water_conservation_error_kg_per_m2)
                  << " environment_weather_simulation_total_evaporated_mg="
                  << kilograms_to_milligrams(plan.total_evaporated_kg)
                  << " environment_weather_simulation_total_condensed_mg="
                  << kilograms_to_milligrams(plan.total_condensed_kg)
                  << " environment_weather_simulation_total_precipitated_mg="
                  << kilograms_to_milligrams(plan.total_precipitated_kg)
                  << " environment_weather_simulation_fallback_cpu_reference_used="
                  << (plan.fallback_cpu_reference_used ? 1 : 0)
                  << " environment_weather_simulation_invokes_gpu=" << (plan.invokes_gpu ? 1 : 0)
                  << " environment_weather_simulation_invokes_backend=" << (plan.invokes_backend ? 1 : 0)
                  << " environment_weather_simulation_native_handle_access=" << (plan.exposes_native_handles ? 1 : 0)
                  << " environment_physical_weather_simulation_ready=" << (plan.physical_weather_ready ? 1 : 0)
                  << " environment_weather_simulation_diagnostics=" << plan.diagnostics.size()
                  << " environment_weather_simulation_replay_hash=" << plan.replay_hash;
    }
    std::cout << '\n';
    print_presentation_report("sample_desktop_runtime_game", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                  << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
        std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                  << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.postprocess_diagnostics()) {
        std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                  << mirakana::win32_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.directional_shadow_diagnostics()) {
        std::cout << "sample_desktop_runtime_game directional_shadow_diagnostic="
                  << mirakana::win32_desktop_presentation_directional_shadow_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.native_ui_overlay_diagnostics()) {
        std::cout << "sample_desktop_runtime_game native_ui_overlay_diagnostic="
                  << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.native_ui_texture_overlay_diagnostics()) {
        std::cout << "sample_desktop_runtime_game native_ui_texture_overlay_diagnostic="
                  << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(diagnostic.status)
                  << ": " << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : framegraph_multiqueue.diagnostics) {
        std::cout << "sample_desktop_runtime_game framegraph_multiqueue_diagnostic=" << diagnostic.message << '\n';
    }
    if (options.require_environment_lighting_vulkan_renderer_execution &&
        !environment_ibl_vulkan_renderer_execution.ready &&
        !environment_ibl_vulkan_renderer_execution.diagnostic.empty()) {
        std::cout << "sample_desktop_runtime_game vulkan_environment_ibl_renderer_diagnostic="
                  << environment_ibl_vulkan_renderer_execution.diagnostic << '\n';
    }

    if (options.smoke) {
        if (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
            game.frames() != options.max_frames) {
            return 3;
        }
        if (!game.hud_passed(options.max_frames)) {
            return 3;
        }
        if (!options.required_scene_package_path.empty() && !game.packaged_scene_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_quaternion_animation && !game.quaternion_animation_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_scene_gpu_bindings &&
            ((scene_gpu_stats.mesh_bindings == 0 && scene_gpu_stats.skinned_mesh_bindings == 0) ||
             scene_gpu_stats.material_bindings == 0 ||
             (scene_gpu_stats.mesh_bindings_resolved + scene_gpu_stats.skinned_mesh_bindings_resolved) !=
                 static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.material_bindings_resolved != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_gpu_skinning && !gpu_skinning_evidence_matches(report, options.max_frames)) {
            return 3;
        }
        if (options.require_d3d12_gpu_skinning_evidence &&
            !d3d12_gpu_skinning_evidence_ready(report, options.max_frames)) {
            return 3;
        }
        if (options.require_vulkan_gpu_skinning_evidence &&
            !vulkan_gpu_skinning_evidence_ready(report, options.max_frames)) {
            return 3;
        }
        if (environment_quality_budget.requested && !environment_quality_budget.ready) {
            return 3;
        }
        if (environment_ready_aggregate.requested && !environment_ready_aggregate.ready) {
            return 3;
        }
        if (environment_vulkan_strict_aggregate.requested && !environment_vulkan_strict_aggregate.ready) {
            return 3;
        }
        if (environment_backend_parity.requested &&
            (environment_backend_parity.plan.status == mirakana::EnvironmentBackendParityStatus::invalid_request ||
             !environment_backend_parity.plan.diagnostics.empty())) {
            return 3;
        }
        if (environment_platform_readiness.requested &&
            (!environment_platform_readiness.windows_d3d12_ready ||
             environment_platform_readiness.windows_vulkan_ready || environment_platform_readiness.linux_vulkan_ready ||
             environment_platform_readiness.macos_metal_ready || environment_platform_readiness.ios_metal_ready ||
             environment_platform_readiness.android_vulkan_ready || environment_platform_readiness.ready ||
             environment_platform_readiness.ready_rows != 1U || environment_platform_readiness.host_gated_rows != 5U ||
             environment_platform_readiness.diagnostics != 0U || environment_platform_readiness.native_handle_access ||
             environment_platform_readiness.invoked_gpu_commands || environment_platform_readiness.replay_hash == 0U)) {
            return 3;
        }
        if (environment_optimization_measurement.requested &&
            (environment_optimization_measurement.plan.status !=
                 mirakana::EnvironmentOptimizationMeasurementStatus::host_evidence_required ||
             !environment_optimization_measurement.plan.diagnostics.empty() ||
             environment_optimization_measurement.plan.row_count != 7U ||
             environment_optimization_measurement.plan.required_workload_count != 7U ||
             environment_optimization_measurement.plan.measured_workload_count != 7U ||
             environment_optimization_measurement.plan.before_after_pair_count != 7U ||
             environment_optimization_measurement.plan.regression_budget_row_count != 7U ||
             environment_optimization_measurement.plan.over_budget_row_count != 0U ||
             !environment_optimization_measurement.plan.d3d12_preset_pack_flythrough_measured ||
             !environment_optimization_measurement.plan.d3d12_storm_precipitation_measured ||
             !environment_optimization_measurement.plan.d3d12_dense_volumetric_fog_measured ||
             !environment_optimization_measurement.plan.d3d12_volumetric_cloud_sunset_measured ||
             !environment_optimization_measurement.plan.d3d12_snowfield_material_weathering_measured ||
             !environment_optimization_measurement.plan.d3d12_weather_simulation_stress_measured ||
             !environment_optimization_measurement.plan.d3d12_asset_library_cold_load_measured ||
             environment_optimization_measurement.plan.environment_backend_parity_ready ||
             environment_optimization_measurement.plan.environment_broad_optimization_ready ||
             environment_optimization_measurement.plan.exposed_native_handles ||
             environment_optimization_measurement.plan.invoked_gpu_commands ||
             environment_optimization_measurement.plan.replay_hash == 0U)) {
            return 3;
        }
        if (environment_weather_simulation_package.requested &&
            (!environment_weather_simulation_package.plan.succeeded() ||
             environment_weather_simulation_package.plan.status !=
                 mirakana::EnvironmentWeatherSimulationStatus::stepped ||
             environment_weather_simulation_package.step_count != 1U ||
             environment_weather_simulation_package.plan.cell_count != 4U ||
             environment_weather_simulation_package.plan.effective_timestep_s != 0.5F ||
             !environment_weather_simulation_package.plan.timestep_clamped ||
             kilograms_to_milligrams(environment_weather_simulation_package.plan.water_conservation_error_kg) >
                 environment_weather_simulation_package_water_error_bound_mg() ||
             !environment_weather_simulation_package.plan.fallback_cpu_reference_used ||
             environment_weather_simulation_package.plan.invokes_gpu ||
             environment_weather_simulation_package.plan.invokes_backend ||
             environment_weather_simulation_package.plan.exposes_native_handles ||
             environment_weather_simulation_package.plan.physical_weather_ready ||
             environment_weather_simulation_package.plan.replay_hash == 0U)) {
            return 3;
        }
        if (options.require_postprocess &&
            (report.postprocess_status != mirakana::Win32DesktopPresentationPostprocessStatus::ready ||
             !postprocess_policy.ready || postprocess_policy.diagnostics_count != 0 ||
             postprocess_policy.effect_count != 1 || postprocess_policy.postprocess_pass_count != 1 ||
             postprocess_policy.framegraph_pass_count != 2 || postprocess_policy.framegraph_barrier_step_budget != 2 ||
             !postprocess_policy.scene_color_required ||
             (!requires_any_environment_fog_postprocess && !postprocess_policy.color_grading_effect) ||
             (requires_any_environment_fog_postprocess && !postprocess_policy.fog_effect) ||
             !postprocess_policy.backend_shader_evidence_ready ||
             (options.require_postprocess_depth_input && !postprocess_policy.scene_depth_required) ||
             report.framegraph_passes != expected_framegraph_passes ||
             report.renderer_stats.framegraph_passes_executed !=
                 static_cast<std::uint64_t>(options.max_frames) * expected_framegraph_passes ||
             report.renderer_stats.framegraph_render_passes_recorded != expected_framegraph_render_passes ||
             report.renderer_stats.framegraph_barrier_steps_executed != expected_framegraph_barrier_steps ||
             report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_postprocess_depth_input && !report.postprocess_depth_input_ready) {
            return 3;
        }
        if (options.require_environment_profile && !environment_profile.ready) {
            return 3;
        }
        if (options.require_environment_texture_asset_pipeline_package && !environment_texture_asset_pipeline.ready) {
            return 3;
        }
        if (options.require_environment_preset_library_package && !environment_preset_library.ready) {
            return 3;
        }
        if (options.require_environment_fog_evidence && !environment_fog.ready) {
            return 3;
        }
        if (options.require_environment_fog_vulkan_package_evidence && !environment_fog_vulkan_package.ready) {
            return 3;
        }
        if (options.require_physical_sky_package_evidence && !physical_sky.ready) {
            return 3;
        }
        if (options.require_physical_sky_vulkan_package_evidence && !physical_sky_vulkan_package.ready) {
            return 3;
        }
        if (options.require_environment_lighting_package_evidence && !environment_lighting.ready) {
            return 3;
        }
        if (options.require_environment_lighting_vulkan_renderer_execution &&
            !environment_ibl_vulkan_renderer_execution.ready) {
            return 3;
        }
        if (options.require_environment_material_weathering && !environment_material_weathering.ready) {
            return 3;
        }
        if (options.require_environment_audio_playback && !environment_audio_playback.ready) {
            return 3;
        }
        if (options.require_environment_volumetric_fog_package_evidence && !environment_volumetric_fog.ready) {
            return 3;
        }
        if (options.require_environment_volumetric_fog_vulkan_renderer_execution &&
            !environment_volumetric_fog_vulkan.ready) {
            return 3;
        }
        if (require_environment_volumetric_cloud && !environment_volumetric_cloud.ready) {
            return 3;
        }
        if (options.require_environment_volumetric_cloud_vulkan_renderer_execution &&
            !environment_volumetric_cloud_vulkan.ready) {
            return 3;
        }
        if (options.require_cloud_layer_package_evidence && !cloud_layer.ready) {
            return 3;
        }
        if (require_environment_any_precipitation_package_evidence && !environment_precipitation.ready) {
            return 3;
        }
        if (options.require_environment_precipitation_vulkan_renderer_execution &&
            !environment_precipitation_vulkan.ready) {
            return 3;
        }
        if (options.require_directional_shadow &&
            (report.directional_shadow_status != mirakana::Win32DesktopPresentationDirectionalShadowStatus::ready ||
             !report.directional_shadow_ready)) {
            return 3;
        }
        if (options.require_directional_shadow_filtering &&
            (report.directional_shadow_filter_mode !=
                 mirakana::Win32DesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||
             report.directional_shadow_filter_tap_count != 9 ||
             report.directional_shadow_filter_radius_texels != 1.0F)) {
            return 3;
        }
        if (options.require_d3d12_shadow_cascade_policy && !d3d12_shadow_cascade_policy_ready(report)) {
            return 3;
        }
        if (options.require_vulkan_shadow_cascade_policy && !vulkan_shadow_cascade_policy_ready(report)) {
            return 3;
        }
        if (options.require_lighting_shadow_policy && !lighting_shadow_policy_ready) {
            return 3;
        }
        if (options.require_scene_scale_policy && !scene_scale_policy.ready) {
            return 3;
        }
        if (options.require_gpu_memory_policy && !gpu_memory_policy.ready) {
            return 3;
        }
        if (options.require_memory_diagnostics &&
            memory_diagnostics.status != mirakana::MemoryDiagnosticsStatus::ready) {
            return 3;
        }
        if (options.require_d3d12_gpu_memory_evidence && !d3d12_gpu_memory_execution.ready) {
            return 3;
        }
        if (options.require_vulkan_gpu_memory_evidence && !vulkan_gpu_memory_execution.ready) {
            return 3;
        }
        if (options.require_debug_profiling_policy && !debug_profiling_policy.ready) {
            return 3;
        }
        if (options.require_d3d12_debug_profiling_evidence && !d3d12_debug_profiling_execution.ready) {
            return 3;
        }
        if (options.require_vulkan_debug_profiling_evidence && !vulkan_debug_profiling_execution.ready) {
            return 3;
        }
        if (options.require_job_scheduling_evidence && !job_scheduling_evidence_ready(job_scheduling_evidence)) {
            return 3;
        }
        if (options.require_job_execution_topology_policy &&
            !job_execution_topology_policy_ready(job_execution_topology_policy)) {
            return 3;
        }
        if (options.require_job_execution_foundation && !job_execution_foundation_ready(job_execution_foundation)) {
            return 3;
        }
        if (options.require_job_execution_work_stealing &&
            !job_execution_work_stealing_ready(job_execution_work_stealing)) {
            return 3;
        }
        if (options.require_job_execution_placement_policy &&
            !job_execution_placement_policy_ready(job_execution_placement_policy)) {
            return 3;
        }
        if (options.require_windows_cpu_set_worker_placement &&
            !windows_cpu_set_worker_placement_ready(windows_cpu_set_worker_placement)) {
            return 3;
        }
        if (options.require_windows_cpu_set_smt_worker_placement &&
            !windows_cpu_set_smt_worker_placement_ready(windows_cpu_set_smt_worker_placement)) {
            return 3;
        }
        if (options.require_simd_dispatch_policy && !simd_dispatch_policy_ready(simd_dispatch_policy)) {
            return 3;
        }
        if (options.require_d3d12_instanced_draw_evidence && !d3d12_instanced_draw_execution.ready) {
            return 3;
        }
        if (options.require_vulkan_instanced_draw_evidence && !vulkan_instanced_draw_execution.ready) {
            return 3;
        }
        if (options.require_d3d12_postprocess_evidence && !d3d12_postprocess_execution.ready) {
            return 3;
        }
        if (options.require_vulkan_postprocess_evidence && !vulkan_postprocess_execution.ready) {
            return 3;
        }
        if (options.require_renderer_quality_gates && !renderer_quality.ready) {
            return 3;
        }
        if (options.require_framegraph_multiqueue_evidence &&
            !d3d12_framegraph_multiqueue_evidence_ready(report, framegraph_multiqueue)) {
            return 3;
        }
        if (options.require_vulkan_framegraph_multiqueue_evidence &&
            !vulkan_framegraph_multiqueue_evidence_ready(report, framegraph_multiqueue)) {
            return 3;
        }
        const auto expected_ui_overlay_sprites =
            static_cast<std::uint64_t>(options.max_frames) *
            static_cast<std::uint64_t>(options.require_native_ui_textured_sprite_atlas ? 2U : 1U);
        if (options.require_native_ui_overlay &&
            (report.native_ui_overlay_status != mirakana::Win32DesktopPresentationNativeUiOverlayStatus::ready ||
             !report.native_ui_overlay_ready ||
             report.native_ui_overlay_sprites_submitted != expected_ui_overlay_sprites ||
             report.native_ui_overlay_draws != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_native_ui_textured_sprite_atlas &&
            (ui_atlas_metadata.status != UiAtlasMetadataStatus::ready || ui_atlas_metadata.pages == 0 ||
             ui_atlas_metadata.bindings == 0)) {
            return 3;
        }
        if (options.require_native_ui_textured_sprite_atlas &&
            (report.native_ui_texture_overlay_status !=
                 mirakana::Win32DesktopPresentationNativeUiTextureOverlayStatus::ready ||
             !report.native_ui_texture_overlay_atlas_ready ||
             report.native_ui_texture_overlay_sprites_submitted != static_cast<std::uint64_t>(options.max_frames) ||
             report.native_ui_texture_overlay_texture_binds != static_cast<std::uint64_t>(options.max_frames) ||
             report.native_ui_texture_overlay_draws != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
    }
    return 0;
}
