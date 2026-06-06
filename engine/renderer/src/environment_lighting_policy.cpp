// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/environment_lighting_policy.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mirakana {
namespace {

constexpr std::uint32_t cubemap_face_count = 6U;
constexpr std::uint32_t supported_sh_order = 3U;
constexpr std::uint32_t supported_sh_coefficient_count = 9U;

void add_diagnostic(EnvironmentLightingPolicyPlan& plan, EnvironmentLightingDiagnosticCode code, std::string field,
                    std::string message) {
    plan.diagnostics.push_back(EnvironmentLightingDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool is_supported_sky_source(EnvironmentLightingSkySource source) noexcept {
    switch (source) {
    case EnvironmentLightingSkySource::physical_sky:
    case EnvironmentLightingSkySource::specified_cubemap:
    case EnvironmentLightingSkySource::solid_color:
        return true;
    case EnvironmentLightingSkySource::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool is_power_of_two(std::uint32_t value) noexcept {
    return value > 0U && (value & (value - 1U)) == 0U;
}

[[nodiscard]] std::uint32_t full_mip_count(std::uint32_t edge_size) noexcept {
    std::uint32_t count = 0U;
    for (auto size = edge_size; size > 0U; size >>= 1U) {
        ++count;
    }
    return count;
}

[[nodiscard]] bool valid_texture_format(EnvironmentLightingTextureFormat format) noexcept {
    switch (format) {
    case EnvironmentLightingTextureFormat::rgba16_float:
    case EnvironmentLightingTextureFormat::rgba32_float:
    case EnvironmentLightingTextureFormat::rgbe9995:
    case EnvironmentLightingTextureFormat::rgba8_unorm:
        return true;
    case EnvironmentLightingTextureFormat::unknown:
        return false;
    }
    return false;
}

void validate_dependency(EnvironmentLightingPolicyPlan& plan, EnvironmentLightingDependencyMode dependency_mode) {
    switch (dependency_mode) {
    case EnvironmentLightingDependencyMode::first_party_cooked_texture:
        return;
    case EnvironmentLightingDependencyMode::unknown:
    case EnvironmentLightingDependencyMode::openexr_import:
    case EnvironmentLightingDependencyMode::ktx_import:
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::unsupported_dependency_request, "dependency_mode",
                       "environment lighting policy uses only first-party cooked texture evidence in this slice");
        return;
    }
}

void validate_reflection_cubemap(EnvironmentLightingPolicyPlan& plan, const EnvironmentReflectionCubemapDesc& cubemap) {
    if (cubemap.asset.value == 0U) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_reflection_cubemap_asset,
                       "reflection_cubemap.asset",
                       "environment lighting requires a valid reflection cubemap asset identifier");
    }
    if (!is_power_of_two(cubemap.edge_size) || cubemap.edge_size < 16U || cubemap.edge_size > 8192U) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_reflection_cubemap_size,
                       "reflection_cubemap.edge_size",
                       "environment lighting cubemap edge size must be a supported power of two");
    }
    if (cubemap.mip_count == 0U || cubemap.mip_count > full_mip_count(cubemap.edge_size)) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_reflection_cubemap_mips,
                       "reflection_cubemap.mip_count",
                       "environment lighting cubemap mip count must fit the full mip chain");
    }
    if (!valid_texture_format(cubemap.format)) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_reflection_cubemap_asset,
                       "reflection_cubemap.format", "environment lighting requires a supported cubemap format");
    }
    if (!cubemap.hdr_metadata_ready) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::missing_hdr_metadata,
                       "reflection_cubemap.hdr_metadata_ready",
                       "environment lighting requires HDR metadata for IBL inputs");
    }
    if (!cubemap.package_evidence_ready) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::missing_package_evidence,
                       "reflection_cubemap.package_evidence_ready",
                       "environment lighting requires package-visible cooked texture evidence");
    }
    if (!cubemap.package_record_ready) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::missing_package_record,
                       "reflection_cubemap.package_record_ready",
                       "environment lighting requires a cooked package index texture record for IBL inputs");
    }
    if (!cubemap.package_source_ready) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::missing_package_source,
                       "reflection_cubemap.package_source_ready",
                       "environment lighting requires first-party cooked texture source metadata for IBL inputs");
    }
}

void validate_irradiance(EnvironmentLightingPolicyPlan& plan, const EnvironmentIrradianceDesc& irradiance) {
    if (irradiance.spherical_harmonic_order != supported_sh_order ||
        irradiance.coefficient_count != supported_sh_coefficient_count) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_irradiance_order, "irradiance",
                       "environment lighting currently supports third-order spherical harmonics with 9 coefficients");
    }
}

void validate_radiance(EnvironmentLightingPolicyPlan& plan, const EnvironmentRadianceDesc& radiance,
                       const EnvironmentReflectionCubemapDesc& cubemap) {
    if (radiance.mip_count == 0U || radiance.mip_count != cubemap.mip_count || radiance.roughness_mip_count == 0U ||
        radiance.roughness_mip_count > radiance.mip_count) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_radiance_mips, "radiance",
                       "environment lighting radiance rows must match the cooked cubemap mip chain");
    }
    if (!std::isfinite(radiance.max_roughness) || radiance.max_roughness <= 0.0F || radiance.max_roughness > 1.0F) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_roughness_policy, "radiance.max_roughness",
                       "environment lighting roughness range must be finite and in (0, 1]");
    }
}

void validate_hdr_clamp(EnvironmentLightingPolicyPlan& plan, const EnvironmentHdrClampPolicyDesc& hdr_clamp) {
    if (!hdr_clamp.enabled) {
        return;
    }
    if (!std::isfinite(hdr_clamp.max_luminance_nits) || hdr_clamp.max_luminance_nits <= 0.0F ||
        !std::isfinite(hdr_clamp.exposure_bias) || hdr_clamp.exposure_bias < -16.0F ||
        hdr_clamp.exposure_bias > 16.0F) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_hdr_clamp, "hdr_clamp",
                       "environment lighting HDR clamp requires finite positive luminance and bounded exposure bias");
    }
}

[[nodiscard]] bool valid_renderer_upload_evidence(const EnvironmentLightingRendererUploadEvidenceDesc& evidence,
                                                  const EnvironmentReflectionCubemapDesc& cubemap,
                                                  const EnvironmentRadianceDesc& radiance,
                                                  const EnvironmentIrradianceDesc& irradiance) noexcept {
    return evidence.evidence_ready && evidence.texture_cube_uploads > 0U &&
           evidence.texture_cube_faces == cubemap_face_count && evidence.texture_cube_edge_size == cubemap.edge_size &&
           evidence.radiance_mips == radiance.mip_count && evidence.irradiance_rows == irradiance.coefficient_count &&
           evidence.shader_sampling_proven && evidence.shader_sample_readback_nonzero;
}

void validate_renderer_upload_evidence(EnvironmentLightingPolicyPlan& plan, const EnvironmentLightingPolicyDesc& desc) {
    if (!desc.request_backend_execution) {
        return;
    }
    if (!valid_renderer_upload_evidence(desc.renderer_upload, desc.reflection_cubemap, desc.radiance,
                                        desc.irradiance)) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::missing_renderer_upload_evidence, "renderer_upload",
                       "environment lighting backend execution requires texture cube upload, TextureCube SRV, "
                       "radiance, irradiance, and shader-readable readback evidence");
    }
}

[[nodiscard]] bool
valid_runtime_capture_evidence(const EnvironmentLightingRuntimeCaptureEvidenceDesc& evidence) noexcept {
    return evidence.evidence_ready && evidence.capture_faces == cubemap_face_count && evidence.readback_nonzero &&
           evidence.readback_checksum != 0U;
}

void validate_runtime_capture_evidence(EnvironmentLightingPolicyPlan& plan, const EnvironmentLightingPolicyDesc& desc) {
    if (!desc.request_runtime_capture) {
        return;
    }
    if (!valid_runtime_capture_evidence(desc.runtime_capture)) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::missing_runtime_capture_evidence, "runtime_capture",
                       "environment lighting runtime capture requires six-face capture and non-zero readback evidence");
    }
}

void append_reflection_cubemap_row(EnvironmentLightingPolicyPlan& plan,
                                   const EnvironmentReflectionCubemapDesc& cubemap) {
    plan.reflection_cubemap_rows.push_back(EnvironmentReflectionCubemapRow{
        .asset = cubemap.asset,
        .face_count = cubemap_face_count,
        .edge_size = cubemap.edge_size,
        .mip_count = cubemap.mip_count,
        .format = cubemap.format,
        .hdr_metadata_ready = cubemap.hdr_metadata_ready,
        .package_record_ready = cubemap.package_record_ready,
        .package_source_ready = cubemap.package_source_ready,
        .package_evidence_ready = cubemap.package_evidence_ready,
    });
}

void append_irradiance_rows(EnvironmentLightingPolicyPlan& plan, const EnvironmentIrradianceDesc& irradiance) {
    for (std::uint32_t coefficient_index = 0U; coefficient_index < irradiance.coefficient_count; ++coefficient_index) {
        plan.irradiance_rows.push_back(EnvironmentIrradianceRow{
            .coefficient_index = coefficient_index,
            .spherical_harmonic_order = irradiance.spherical_harmonic_order,
        });
    }
}

void append_radiance_rows(EnvironmentLightingPolicyPlan& plan, const EnvironmentRadianceDesc& radiance,
                          const EnvironmentReflectionCubemapDesc& cubemap) {
    const auto roughness_denominator = radiance.roughness_mip_count > 1U ? radiance.roughness_mip_count - 1U : 1U;
    for (std::uint32_t mip_index = 0U; mip_index < radiance.mip_count; ++mip_index) {
        const auto edge_size = std::max(1U, cubemap.edge_size >> mip_index);
        const auto roughness_index = std::min(mip_index, radiance.roughness_mip_count - 1U);
        const auto roughness =
            radiance.max_roughness * static_cast<float>(roughness_index) / static_cast<float>(roughness_denominator);
        plan.radiance_mip_rows.push_back(EnvironmentRadianceMipRow{
            .mip_index = mip_index,
            .edge_size = edge_size,
            .roughness = roughness,
        });
    }
}

void append_package_evidence_row(EnvironmentLightingPolicyPlan& plan, const EnvironmentReflectionCubemapDesc& cubemap) {
    plan.package_evidence_rows.push_back(EnvironmentLightingPackageEvidenceRow{
        .asset = cubemap.asset,
        .evidence_kind = "first_party_cooked_texture_hdr_metadata",
        .package_record_ready = cubemap.package_record_ready,
        .package_source_ready = cubemap.package_source_ready,
        .hdr_metadata_ready = cubemap.hdr_metadata_ready,
        .renderer_upload_evidence_ready = plan.renderer_upload_evidence_ready,
        .ready = cubemap.package_evidence_ready && cubemap.package_record_ready && cubemap.package_source_ready &&
                 cubemap.hdr_metadata_ready,
    });
}

} // namespace

bool EnvironmentLightingPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

EnvironmentLightingPolicyPlan plan_environment_lighting_policy(const EnvironmentLightingPolicyDesc& desc) {
    EnvironmentLightingPolicyPlan plan{
        .dependency_mode = desc.dependency_mode,
        .visual_sky_source = desc.visual_sky_source,
        .lighting_sky_source = desc.lighting_sky_source,
        .visual_sky_visible_in_main_camera = desc.visual_sky_visible_in_main_camera,
        .hdr_clamp_row =
            EnvironmentHdrClampPolicyRow{
                .enabled = desc.hdr_clamp.enabled,
                .max_luminance_nits = desc.hdr_clamp.max_luminance_nits,
                .exposure_bias = desc.hdr_clamp.exposure_bias,
            },
    };

    validate_dependency(plan, desc.dependency_mode);
    if (!is_supported_sky_source(desc.visual_sky_source)) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_visual_sky_source, "visual_sky_source",
                       "environment lighting requires a supported visual sky source");
    }
    if (!is_supported_sky_source(desc.lighting_sky_source)) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::invalid_lighting_sky_source, "lighting_sky_source",
                       "environment lighting requires a supported lighting sky source");
    }
    validate_reflection_cubemap(plan, desc.reflection_cubemap);
    validate_irradiance(plan, desc.irradiance);
    validate_radiance(plan, desc.radiance, desc.reflection_cubemap);
    validate_hdr_clamp(plan, desc.hdr_clamp);
    validate_renderer_upload_evidence(plan, desc);
    validate_runtime_capture_evidence(plan, desc);

    if (desc.request_coupled_visual_and_lighting_sky) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::unsupported_visual_lighting_coupling,
                       "request_coupled_visual_and_lighting_sky",
                       "environment lighting keeps visual sky and lighting source independently declared");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentLightingDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access",
                       "environment lighting policy planning must not expose native renderer or RHI handles");
    }

    if (plan.succeeded()) {
        if (desc.request_backend_execution) {
            plan.allocates_textures = true;
            plan.invokes_backend = true;
            plan.renderer_upload_evidence_ready = true;
            plan.texture_uploads = desc.renderer_upload.texture_cube_uploads;
            plan.texture_cube_uploads = desc.renderer_upload.texture_cube_uploads;
            plan.texture_cube_faces = desc.renderer_upload.texture_cube_faces;
            plan.texture_cube_edge_size = desc.renderer_upload.texture_cube_edge_size;
            plan.renderer_radiance_mips = desc.renderer_upload.radiance_mips;
            plan.renderer_irradiance_rows = desc.renderer_upload.irradiance_rows;
            plan.shader_sampling_proven = desc.renderer_upload.shader_sampling_proven;
            plan.shader_sample_readback_nonzero = desc.renderer_upload.shader_sample_readback_nonzero;
            plan.backend_invocations = 1U;
        }
        if (desc.request_runtime_capture) {
            plan.invokes_backend = true;
            plan.runtime_capture_evidence_ready = true;
            plan.runtime_captures = 1U;
            plan.runtime_capture_faces = desc.runtime_capture.capture_faces;
            plan.runtime_capture_readback_nonzero = desc.runtime_capture.readback_nonzero;
            plan.runtime_capture_readback_checksum = desc.runtime_capture.readback_checksum;
            if (plan.backend_invocations == 0U) {
                plan.backend_invocations = 1U;
            }
        }
        append_reflection_cubemap_row(plan, desc.reflection_cubemap);
        append_irradiance_rows(plan, desc.irradiance);
        append_radiance_rows(plan, desc.radiance, desc.reflection_cubemap);
        append_package_evidence_row(plan, desc.reflection_cubemap);
    }

    return plan;
}

bool has_environment_lighting_diagnostic(const EnvironmentLightingPolicyPlan& plan,
                                         EnvironmentLightingDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
