// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/volumetric_cloud_policy.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr float max_altitude_m = 120000.0F;
constexpr float max_abs_wind_mps = 500.0F;
constexpr std::uint32_t max_primary_steps = 512U;
constexpr std::uint32_t max_light_steps = 128U;
constexpr float max_lightning_flash_intensity = 250000.0F;
constexpr float max_thunder_delay_seconds = 60.0F;
constexpr float max_wind_gust_mps = 250.0F;

void add_diagnostic(VolumetricCloudPolicyPlan& plan, VolumetricCloudDiagnosticCode code, std::string field,
                    std::uint32_t source_index, std::string message) {
    plan.diagnostics.push_back(VolumetricCloudDiagnostic{
        .code = code,
        .field = std::move(field),
        .source_index = source_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool valid_unit_color(Vec3 value) noexcept {
    return finite_in_range(value.x, 0.0F, 1.0F) && finite_in_range(value.y, 0.0F, 1.0F) &&
           finite_in_range(value.z, 0.0F, 1.0F);
}

[[nodiscard]] bool valid_direction(Vec3 direction) noexcept {
    if (!finite_vec3(direction)) {
        return false;
    }
    constexpr float minimum_direction_length_squared = 0.0001F;
    return dot(direction, direction) >= minimum_direction_length_squared;
}

[[nodiscard]] bool contains_forbidden_token(std::string_view value) noexcept {
    constexpr std::string_view forbidden_tokens[] = {
        "native", "backend", "d3d", "d3d12", "dxgi", "vulkan", "metal", "imgui", "sdl", "sdl3",
    };
    return std::ranges::any_of(forbidden_tokens,
                               [value](std::string_view token) { return value.find(token) != std::string_view::npos; });
}

[[nodiscard]] bool valid_asset_reference(std::string_view value) noexcept {
    if (value.empty() || value.size() > 256U || contains_forbidden_token(value) ||
        value.find("..") != std::string_view::npos) {
        return false;
    }
    return std::ranges::all_of(value, [](char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '/';
    });
}

[[nodiscard]] bool valid_quality_tier(VolumetricCloudQualityTier tier) noexcept {
    switch (tier) {
    case VolumetricCloudQualityTier::low:
    case VolumetricCloudQualityTier::balanced:
    case VolumetricCloudQualityTier::high:
    case VolumetricCloudQualityTier::cinematic:
    case VolumetricCloudQualityTier::custom:
        return true;
    case VolumetricCloudQualityTier::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool valid_shadow_mode(VolumetricCloudShadowMode mode) noexcept {
    switch (mode) {
    case VolumetricCloudShadowMode::none:
    case VolumetricCloudShadowMode::beer_shadow_map_intent:
    case VolumetricCloudShadowMode::raymarched_secondary:
        return true;
    case VolumetricCloudShadowMode::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool valid_light(const VolumetricCloudAtmosphericLightDesc& light) noexcept {
    return valid_direction(light.direction) && valid_unit_color(light.color) && std::isfinite(light.illuminance_lux) &&
           light.illuminance_lux >= 0.0F;
}

[[nodiscard]] bool valid_storm_desc(const VolumetricCloudStormDesc& storm) noexcept {
    if (!storm.enabled) {
        return true;
    }
    return finite_in_range(storm.lightning_flash_intensity, 0.0F, max_lightning_flash_intensity) &&
           valid_direction(storm.lightning_direction) &&
           finite_in_range(storm.thunder_delay_seconds, 0.0F, max_thunder_delay_seconds) &&
           finite_in_range(storm.cloud_darkening, 0.0F, 1.0F) &&
           finite_in_range(storm.precipitation_boost, 0.0F, 1.0F) &&
           finite_in_range(storm.wind_gust_mps, 0.0F, max_wind_gust_mps) &&
           finite_in_range(storm.exposure_response, 0.0F, 1.0F);
}

void validate_desc(VolumetricCloudPolicyPlan& plan, const VolumetricCloudPolicyDesc& desc) {
    if (!valid_asset_reference(desc.layer.weather_map_asset_ref)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_weather_map_reference,
                       "layer.weather_map_asset_ref", 0U,
                       "volumetric cloud weather map reference must be a first-party asset id");
    }
    if (!valid_asset_reference(desc.layer.shape_noise_asset_ref)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_shape_noise_reference,
                       "layer.shape_noise_asset_ref", 0U,
                       "volumetric cloud shape noise reference must be a first-party asset id");
    }
    if (!valid_asset_reference(desc.layer.erosion_noise_asset_ref)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_erosion_noise_reference,
                       "layer.erosion_noise_asset_ref", 0U,
                       "volumetric cloud erosion noise reference must be a first-party asset id");
    }
    if (!finite_in_range(desc.layer.coverage, 0.0F, 1.0F)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_coverage, "layer.coverage", 0U,
                       "volumetric cloud coverage must be finite and in [0, 1]");
    }
    if (!finite_in_range(desc.layer.density, 0.0F, 1.0F)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_density, "layer.density", 0U,
                       "volumetric cloud density must be finite and in [0, 1]");
    }
    if (!std::isfinite(desc.layer.altitude_min_m) || !std::isfinite(desc.layer.altitude_max_m) ||
        desc.layer.altitude_min_m < 0.0F || desc.layer.altitude_max_m <= desc.layer.altitude_min_m ||
        desc.layer.altitude_max_m > max_altitude_m) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_altitude_range, "layer.altitude", 0U,
                       "volumetric cloud altitude range must be finite, positive, ordered, and bounded");
    }
    if (!finite_in_range(desc.layer.wind_velocity_mps.x, -max_abs_wind_mps, max_abs_wind_mps) ||
        !finite_in_range(desc.layer.wind_velocity_mps.y, -max_abs_wind_mps, max_abs_wind_mps)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_wind_velocity, "layer.wind_velocity_mps", 0U,
                       "volumetric cloud wind velocity must be finite and bounded");
    }
    if (!valid_quality_tier(desc.quality_tier)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::unsupported_quality_tier, "quality_tier", 0U,
                       "volumetric cloud policy requires a supported quality tier");
    }
    if (desc.atmospheric_lights.empty() || desc.atmospheric_lights.size() > 2U) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_lighting_source_count, "atmospheric_lights", 0U,
                       "volumetric cloud policy supports one or two atmospheric directional lights");
    }
    for (const auto& light : desc.atmospheric_lights) {
        if (!valid_light(light)) {
            add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_light, "atmospheric_lights", light.source_index,
                           "volumetric cloud atmospheric lights require finite direction, color, and illuminance");
        }
    }
    if (desc.raymarch.primary_steps == 0U || desc.raymarch.primary_steps > max_primary_steps ||
        desc.raymarch.light_steps > max_light_steps) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_raymarch_budget, "raymarch", 0U,
                       "volumetric cloud raymarch budgets must be bounded");
    }
    if (!valid_shadow_mode(desc.raymarch.shadow_mode)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_cloud_shadow, "raymarch.shadow_mode", 0U,
                       "volumetric cloud shadow mode must be reviewed");
    }
    if (!finite_in_range(desc.raymarch.temporal_history_weight, 0.0F, 1.0F)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_temporal_reprojection,
                       "raymarch.temporal_history_weight", 0U,
                       "volumetric cloud temporal history weight must be finite and in [0, 1]");
    }
    if (!valid_storm_desc(desc.storm)) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::invalid_storm_lightning, "storm", 0U,
                       "storm lightning rows require finite bounded flash, direction, delay, darkening, boost, gust, "
                       "and exposure values");
    }
    if (!desc.shader_contract_evidence_ready) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::missing_shader_contract_evidence,
                       "shader_contract_evidence_ready", 0U,
                       "volumetric cloud policy requires validated shader contract evidence");
    }
    if (desc.request_ready_promotion && !desc.execution_evidence_ready) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::missing_execution_evidence, "execution_evidence_ready", 0U,
                       "volumetric cloud ready promotion requires backend or package execution evidence");
    }
    if (desc.request_volume_texture_upload) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::unsupported_volume_texture_upload,
                       "request_volume_texture_upload", 0U,
                       "volumetric cloud policy planning must not upload textures in this slice");
    }
    if (desc.request_backend_execution) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::unsupported_backend_execution, "request_backend_execution",
                       0U, "volumetric cloud policy planning must not invoke renderer or RHI backends in this slice");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access", 0U,
                       "volumetric cloud policy planning must not expose native renderer or RHI handles");
    }
    if (desc.request_audio_playback) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::unsupported_audio_playback, "request_audio_playback", 0U,
                       "storm rows may hand off thunder delay intent but must not play audio");
    }
    if (desc.request_precipitation_execution) {
        add_diagnostic(plan, VolumetricCloudDiagnosticCode::unsupported_precipitation_execution,
                       "request_precipitation_execution", 0U,
                       "storm rows may boost precipitation intent but must not execute precipitation rendering");
    }
}

void append_rows(VolumetricCloudPolicyPlan& plan, const VolumetricCloudPolicyDesc& desc) {
    plan.map_rows.push_back(VolumetricCloudMapRow{
        .weather_map_asset_ref = desc.layer.weather_map_asset_ref,
        .shape_noise_asset_ref = desc.layer.shape_noise_asset_ref,
        .erosion_noise_asset_ref = desc.layer.erosion_noise_asset_ref,
        .weather_map_binding_slot = volumetric_cloud_weather_map_binding(),
        .shape_noise_binding_slot = volumetric_cloud_shape_noise_binding(),
        .erosion_noise_binding_slot = volumetric_cloud_erosion_noise_binding(),
    });
    plan.layer_rows.push_back(VolumetricCloudLayerRow{
        .coverage = desc.layer.coverage,
        .density = desc.layer.density,
        .altitude_min_m = desc.layer.altitude_min_m,
        .altitude_max_m = desc.layer.altitude_max_m,
        .wind_velocity_mps = desc.layer.wind_velocity_mps,
    });
    for (const auto& light : desc.atmospheric_lights) {
        plan.lighting_rows.push_back(VolumetricCloudLightingRow{
            .direction = light.direction,
            .color = light.color,
            .illuminance_lux = light.illuminance_lux,
            .casts_cloud_shadows = light.casts_cloud_shadows,
            .source_index = light.source_index,
        });
    }
    plan.raymarch_rows.push_back(VolumetricCloudRaymarchRow{
        .primary_steps = desc.raymarch.primary_steps,
        .light_steps = desc.raymarch.light_steps,
        .shadow_mode = desc.raymarch.shadow_mode,
    });
    plan.temporal_rows.push_back(VolumetricCloudTemporalRow{
        .enabled = desc.raymarch.temporal_reprojection_enabled,
        .history_weight = desc.raymarch.temporal_history_weight,
    });
    plan.shadow_rows.push_back(VolumetricCloudShadowRow{
        .mode = desc.raymarch.shadow_mode,
        .casts_ground_shadows = desc.raymarch.shadow_mode != VolumetricCloudShadowMode::none,
        .casts_self_shadows = desc.raymarch.shadow_mode == VolumetricCloudShadowMode::raymarched_secondary,
    });
    if (desc.storm.enabled) {
        plan.storm_rows.push_back(VolumetricCloudStormRow{
            .lightning_flash_intensity = desc.storm.lightning_flash_intensity,
            .lightning_direction = desc.storm.lightning_direction,
            .thunder_delay_seconds = desc.storm.thunder_delay_seconds,
            .cloud_darkening = desc.storm.cloud_darkening,
            .precipitation_boost = desc.storm.precipitation_boost,
            .wind_gust_mps = desc.storm.wind_gust_mps,
            .exposure_response = desc.storm.exposure_response,
        });
    }
    plan.shader_contract_rows.push_back(VolumetricCloudShaderContractRow{
        .constants_binding_slot = volumetric_cloud_constants_binding(),
        .sampler_binding_slot = volumetric_cloud_sampler_binding(),
        .samples_weather_map = true,
        .samples_shape_noise = true,
        .samples_erosion_noise = true,
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
    });
    plan.quality_rows.push_back(VolumetricCloudQualityRow{
        .tier = desc.quality_tier,
        .primary_steps = desc.raymarch.primary_steps,
        .light_steps = desc.raymarch.light_steps,
        .ready = plan.ready(),
    });
}

} // namespace

bool VolumetricCloudPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

bool VolumetricCloudPolicyPlan::ready() const noexcept {
    return status == VolumetricCloudPolicyStatus::ready;
}

VolumetricCloudPolicyPlan plan_volumetric_cloud_policy(const VolumetricCloudPolicyDesc& desc) {
    VolumetricCloudPolicyPlan plan{
        .status = VolumetricCloudPolicyStatus::planned,
        .uses_volumetric_clouds = true,
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
        .execution_evidence_ready = desc.execution_evidence_ready,
    };

    validate_desc(plan, desc);

    if (!plan.succeeded()) {
        plan.status = VolumetricCloudPolicyStatus::blocked;
    } else if (desc.request_ready_promotion && desc.execution_evidence_ready) {
        plan.status = VolumetricCloudPolicyStatus::ready;
    }

    if (plan.succeeded()) {
        append_rows(plan, desc);
    }

    return plan;
}

bool has_volumetric_cloud_diagnostic(const VolumetricCloudPolicyPlan& plan,
                                     VolumetricCloudDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
