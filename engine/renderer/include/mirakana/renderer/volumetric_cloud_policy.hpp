// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class VolumetricCloudQualityTier : std::uint8_t {
    unknown = 0,
    low,
    balanced,
    high,
    cinematic,
    custom,
};

enum class VolumetricCloudShadowMode : std::uint8_t {
    unknown = 0,
    none,
    beer_shadow_map_intent,
    raymarched_secondary,
};

enum class VolumetricCloudPolicyStatus : std::uint8_t {
    blocked = 0,
    planned,
    ready,
};

enum class VolumetricCloudDiagnosticCode : std::uint8_t {
    none = 0,
    unsupported_quality_tier,
    invalid_weather_map_reference,
    invalid_shape_noise_reference,
    invalid_erosion_noise_reference,
    invalid_coverage,
    invalid_density,
    invalid_altitude_range,
    invalid_wind_velocity,
    invalid_lighting_source_count,
    invalid_light,
    invalid_raymarch_budget,
    invalid_temporal_reprojection,
    invalid_cloud_shadow,
    invalid_storm_lightning,
    missing_shader_contract_evidence,
    missing_execution_evidence,
    unsupported_volume_texture_upload,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
    unsupported_audio_playback,
    unsupported_precipitation_execution,
};

[[nodiscard]] constexpr std::uint32_t volumetric_cloud_weather_map_binding() noexcept {
    return 10;
}

[[nodiscard]] constexpr std::uint32_t volumetric_cloud_shape_noise_binding() noexcept {
    return 11;
}

[[nodiscard]] constexpr std::uint32_t volumetric_cloud_erosion_noise_binding() noexcept {
    return 12;
}

[[nodiscard]] constexpr std::uint32_t volumetric_cloud_sampler_binding() noexcept {
    return 10;
}

[[nodiscard]] constexpr std::uint32_t volumetric_cloud_constants_binding() noexcept {
    return 8;
}

struct VolumetricCloudLayerDesc {
    std::string weather_map_asset_ref;
    std::string shape_noise_asset_ref;
    std::string erosion_noise_asset_ref;
    float coverage{0.0F};
    float density{0.0F};
    float altitude_min_m{1000.0F};
    float altitude_max_m{8000.0F};
    Vec2 wind_velocity_mps{};
};

struct VolumetricCloudRaymarchBudgetDesc {
    std::uint32_t primary_steps{64};
    std::uint32_t light_steps{8};
    VolumetricCloudShadowMode shadow_mode{VolumetricCloudShadowMode::beer_shadow_map_intent};
    bool temporal_reprojection_enabled{true};
    float temporal_history_weight{0.9F};
};

struct VolumetricCloudAtmosphericLightDesc {
    Vec3 direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    Vec3 color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float illuminance_lux{100000.0F};
    bool casts_cloud_shadows{true};
    std::uint32_t source_index{0};
};

struct VolumetricCloudStormDesc {
    bool enabled{false};
    float lightning_flash_intensity{0.0F};
    Vec3 lightning_direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    float thunder_delay_seconds{0.0F};
    float cloud_darkening{0.0F};
    float precipitation_boost{0.0F};
    float wind_gust_mps{0.0F};
    float exposure_response{0.0F};
};

struct VolumetricCloudPolicyDesc {
    VolumetricCloudLayerDesc layer;
    VolumetricCloudQualityTier quality_tier{VolumetricCloudQualityTier::balanced};
    VolumetricCloudRaymarchBudgetDesc raymarch;
    std::span<const VolumetricCloudAtmosphericLightDesc> atmospheric_lights;
    VolumetricCloudStormDesc storm;
    bool shader_contract_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool request_ready_promotion{false};
    bool request_volume_texture_upload{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
    bool request_audio_playback{false};
    bool request_precipitation_execution{false};
};

struct VolumetricCloudMapRow {
    std::string weather_map_asset_ref;
    std::string shape_noise_asset_ref;
    std::string erosion_noise_asset_ref;
    std::uint32_t weather_map_binding_slot{0};
    std::uint32_t shape_noise_binding_slot{0};
    std::uint32_t erosion_noise_binding_slot{0};
};

struct VolumetricCloudLayerRow {
    float coverage{0.0F};
    float density{0.0F};
    float altitude_min_m{0.0F};
    float altitude_max_m{0.0F};
    Vec2 wind_velocity_mps{};
};

struct VolumetricCloudLightingRow {
    Vec3 direction{};
    Vec3 color{};
    float illuminance_lux{0.0F};
    bool casts_cloud_shadows{false};
    std::uint32_t source_index{0};
};

struct VolumetricCloudRaymarchRow {
    std::uint32_t primary_steps{0};
    std::uint32_t light_steps{0};
    VolumetricCloudShadowMode shadow_mode{VolumetricCloudShadowMode::unknown};
};

struct VolumetricCloudTemporalRow {
    bool enabled{false};
    float history_weight{0.0F};
};

struct VolumetricCloudShadowRow {
    VolumetricCloudShadowMode mode{VolumetricCloudShadowMode::unknown};
    bool casts_ground_shadows{false};
    bool casts_self_shadows{false};
};

struct VolumetricCloudStormRow {
    float lightning_flash_intensity{0.0F};
    Vec3 lightning_direction{};
    float thunder_delay_seconds{0.0F};
    float cloud_darkening{0.0F};
    float precipitation_boost{0.0F};
    float wind_gust_mps{0.0F};
    float exposure_response{0.0F};
};

struct VolumetricCloudShaderContractRow {
    std::uint32_t constants_binding_slot{0};
    std::uint32_t sampler_binding_slot{0};
    bool samples_weather_map{false};
    bool samples_shape_noise{false};
    bool samples_erosion_noise{false};
    bool shader_contract_evidence_ready{false};
};

struct VolumetricCloudQualityRow {
    VolumetricCloudQualityTier tier{VolumetricCloudQualityTier::unknown};
    std::uint32_t primary_steps{0};
    std::uint32_t light_steps{0};
    bool ready{false};
};

struct VolumetricCloudDiagnostic {
    VolumetricCloudDiagnosticCode code{VolumetricCloudDiagnosticCode::none};
    std::string field;
    std::uint32_t source_index{0};
    std::string message;
};

struct VolumetricCloudPolicyPlan {
    VolumetricCloudPolicyStatus status{VolumetricCloudPolicyStatus::blocked};
    bool uses_volumetric_clouds{false};
    bool shader_contract_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool uploads_volume_textures{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool plays_audio{false};
    bool executes_precipitation{false};
    std::vector<VolumetricCloudMapRow> map_rows;
    std::vector<VolumetricCloudLayerRow> layer_rows;
    std::vector<VolumetricCloudLightingRow> lighting_rows;
    std::vector<VolumetricCloudRaymarchRow> raymarch_rows;
    std::vector<VolumetricCloudTemporalRow> temporal_rows;
    std::vector<VolumetricCloudShadowRow> shadow_rows;
    std::vector<VolumetricCloudStormRow> storm_rows;
    std::vector<VolumetricCloudShaderContractRow> shader_contract_rows;
    std::vector<VolumetricCloudQualityRow> quality_rows;
    std::vector<VolumetricCloudDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] VolumetricCloudPolicyPlan plan_volumetric_cloud_policy(const VolumetricCloudPolicyDesc& desc);

[[nodiscard]] bool has_volumetric_cloud_diagnostic(const VolumetricCloudPolicyPlan& plan,
                                                   VolumetricCloudDiagnosticCode code) noexcept;

} // namespace mirakana
