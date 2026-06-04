// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/cloud_layer.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class CloudLayerQualityTier : std::uint8_t {
    unknown = 0,
    low,
    balanced,
    high,
    custom,
};

enum class CloudLayerPolicyStatus : std::uint8_t {
    blocked = 0,
    planned,
    ready,
};

enum class CloudLayerDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_environment_layer,
    unsupported_quality_tier,
    missing_shader_contract_evidence,
    missing_package_evidence,
    missing_execution_evidence,
    unsupported_texture_upload,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
    unsupported_volumetric_clouds,
};

[[nodiscard]] constexpr std::uint32_t cloud_layer_cloud_map_binding() noexcept {
    return 6;
}

[[nodiscard]] constexpr std::uint32_t cloud_layer_flow_map_binding() noexcept {
    return 7;
}

[[nodiscard]] constexpr std::uint32_t cloud_layer_sampler_binding() noexcept {
    return 6;
}

[[nodiscard]] constexpr std::uint32_t cloud_layer_constants_binding() noexcept {
    return 6;
}

struct CloudLayerPolicyDesc {
    EnvironmentCloudLayerDesc layer;
    CloudLayerQualityTier quality_tier{CloudLayerQualityTier::balanced};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool request_ready_promotion{false};
    bool request_texture_upload{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
    bool request_volumetric_clouds{false};
};

struct CloudLayerTextureRow {
    std::string cloud_map_asset_ref;
    std::string flow_map_asset_ref;
    std::uint32_t cloud_map_binding_slot{0};
    std::uint32_t flow_map_binding_slot{0};
    std::uint32_t sampler_binding_slot{0};
    bool package_evidence_ready{false};
    bool uploads_texture{false};
};

struct CloudLayerVisualRow {
    float coverage{0.0F};
    float opacity{0.0F};
    float altitude_m{0.0F};
    Vec2 wind_velocity_mps{};
    Vec3 sky_tint_response{};
    float time_of_day_response{0.0F};
};

struct CloudLayerIblRow {
    EnvironmentCloudIblContributionMode mode{EnvironmentCloudIblContributionMode::unknown};
    float contribution{0.0F};
};

struct CloudLayerShaderContractRow {
    std::uint32_t constants_binding_slot{0};
    bool uses_latlong_projection{false};
    bool uses_flow_map{false};
    bool shader_contract_evidence_ready{false};
};

struct CloudLayerQualityRow {
    CloudLayerQualityTier tier{CloudLayerQualityTier::unknown};
    bool default_lower_tier_cloud_path{false};
    bool ready{false};
};

struct CloudLayerDiagnostic {
    CloudLayerDiagnosticCode code{CloudLayerDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct CloudLayerPolicyPlan {
    CloudLayerPolicyStatus status{CloudLayerPolicyStatus::blocked};
    bool uses_2d_cloud_layer{false};
    bool uses_volumetric_clouds{false};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool uploads_textures{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::vector<CloudLayerTextureRow> texture_rows;
    std::vector<CloudLayerVisualRow> visual_rows;
    std::vector<CloudLayerIblRow> ibl_rows;
    std::vector<CloudLayerShaderContractRow> shader_contract_rows;
    std::vector<CloudLayerQualityRow> quality_rows;
    std::vector<CloudLayerDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] CloudLayerPolicyPlan plan_cloud_layer_policy(const CloudLayerPolicyDesc& desc);

[[nodiscard]] bool has_cloud_layer_diagnostic(const CloudLayerPolicyPlan& plan, CloudLayerDiagnosticCode code) noexcept;

} // namespace mirakana
