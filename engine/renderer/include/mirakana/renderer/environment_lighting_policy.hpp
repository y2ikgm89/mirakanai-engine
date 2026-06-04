// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentLightingDependencyMode : std::uint8_t {
    unknown = 0,
    first_party_cooked_texture,
    openexr_import,
    ktx_import,
};

enum class EnvironmentLightingSkySource : std::uint8_t {
    unknown = 0,
    physical_sky,
    specified_cubemap,
    solid_color,
};

enum class EnvironmentLightingTextureFormat : std::uint8_t {
    unknown = 0,
    rgba16_float,
    rgba32_float,
    rgbe9995,
    rgba8_unorm,
};

enum class EnvironmentLightingDiagnosticCode : std::uint8_t {
    none = 0,
    unsupported_dependency_request,
    invalid_visual_sky_source,
    invalid_lighting_sky_source,
    invalid_reflection_cubemap_asset,
    invalid_reflection_cubemap_size,
    invalid_reflection_cubemap_mips,
    missing_hdr_metadata,
    missing_package_evidence,
    invalid_irradiance_order,
    invalid_radiance_mips,
    invalid_roughness_policy,
    invalid_hdr_clamp,
    unsupported_visual_lighting_coupling,
    unsupported_runtime_capture,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
};

struct EnvironmentReflectionCubemapDesc {
    AssetId asset;
    std::uint32_t edge_size{0};
    std::uint32_t mip_count{0};
    EnvironmentLightingTextureFormat format{EnvironmentLightingTextureFormat::unknown};
    bool hdr_metadata_ready{false};
    bool package_evidence_ready{false};
};

struct EnvironmentIrradianceDesc {
    std::uint32_t spherical_harmonic_order{3};
    std::uint32_t coefficient_count{9};
};

struct EnvironmentRadianceDesc {
    std::uint32_t mip_count{0};
    std::uint32_t roughness_mip_count{0};
    float max_roughness{1.0F};
};

struct EnvironmentHdrClampPolicyDesc {
    bool enabled{false};
    float max_luminance_nits{0.0F};
    float exposure_bias{0.0F};
};

struct EnvironmentLightingPolicyDesc {
    EnvironmentLightingDependencyMode dependency_mode{EnvironmentLightingDependencyMode::unknown};
    EnvironmentLightingSkySource visual_sky_source{EnvironmentLightingSkySource::unknown};
    EnvironmentLightingSkySource lighting_sky_source{EnvironmentLightingSkySource::unknown};
    EnvironmentReflectionCubemapDesc reflection_cubemap;
    EnvironmentIrradianceDesc irradiance;
    EnvironmentRadianceDesc radiance;
    EnvironmentHdrClampPolicyDesc hdr_clamp;
    bool visual_sky_visible_in_main_camera{true};
    bool request_coupled_visual_and_lighting_sky{false};
    bool request_runtime_capture{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
};

struct EnvironmentReflectionCubemapRow {
    AssetId asset;
    std::uint32_t face_count{6};
    std::uint32_t edge_size{0};
    std::uint32_t mip_count{0};
    EnvironmentLightingTextureFormat format{EnvironmentLightingTextureFormat::unknown};
    bool hdr_metadata_ready{false};
    bool package_evidence_ready{false};
    bool allocates_texture{false};
};

struct EnvironmentIrradianceRow {
    std::uint32_t coefficient_index{0};
    std::uint32_t spherical_harmonic_order{0};
};

struct EnvironmentRadianceMipRow {
    std::uint32_t mip_index{0};
    std::uint32_t edge_size{0};
    float roughness{0.0F};
};

struct EnvironmentLightingPackageEvidenceRow {
    AssetId asset;
    std::string evidence_kind;
    bool ready{false};
};

struct EnvironmentHdrClampPolicyRow {
    bool enabled{false};
    float max_luminance_nits{0.0F};
    float exposure_bias{0.0F};
};

struct EnvironmentLightingDiagnostic {
    EnvironmentLightingDiagnosticCode code{EnvironmentLightingDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct EnvironmentLightingPolicyPlan {
    EnvironmentLightingDependencyMode dependency_mode{EnvironmentLightingDependencyMode::unknown};
    EnvironmentLightingSkySource visual_sky_source{EnvironmentLightingSkySource::unknown};
    EnvironmentLightingSkySource lighting_sky_source{EnvironmentLightingSkySource::unknown};
    bool visual_sky_visible_in_main_camera{true};
    bool requires_visual_sky_visible_in_main_camera{false};
    bool uses_new_import_dependency{false};
    bool allocates_textures{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::vector<EnvironmentReflectionCubemapRow> reflection_cubemap_rows;
    std::vector<EnvironmentIrradianceRow> irradiance_rows;
    std::vector<EnvironmentRadianceMipRow> radiance_mip_rows;
    std::vector<EnvironmentLightingPackageEvidenceRow> package_evidence_rows;
    EnvironmentHdrClampPolicyRow hdr_clamp_row;
    std::vector<EnvironmentLightingDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentLightingPolicyPlan plan_environment_lighting_policy(const EnvironmentLightingPolicyDesc& desc);

[[nodiscard]] bool has_environment_lighting_diagnostic(const EnvironmentLightingPolicyPlan& plan,
                                                       EnvironmentLightingDiagnosticCode code) noexcept;

} // namespace mirakana
