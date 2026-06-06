// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/precipitation_policy.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(PrecipitationPolicyPlan& plan, PrecipitationPolicyDiagnosticCode code, std::string field,
                    std::string message) {
    plan.diagnostics.push_back(PrecipitationPolicyDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool valid_quality_tier(PrecipitationQualityTier tier) noexcept {
    switch (tier) {
    case PrecipitationQualityTier::low:
    case PrecipitationQualityTier::balanced:
    case PrecipitationQualityTier::high:
    case PrecipitationQualityTier::custom:
        return true;
    case PrecipitationQualityTier::unknown:
        return false;
    }
    return false;
}

void write_f32(std::span<std::uint8_t> destination, std::size_t offset, float value) {
    std::memcpy(destination.data() + offset, &value, sizeof(float));
}

[[nodiscard]] float precipitation_kind_id(EnvironmentPrecipitationKind kind) noexcept {
    switch (kind) {
    case EnvironmentPrecipitationKind::none:
        return 0.0F;
    case EnvironmentPrecipitationKind::rain:
        return 1.0F;
    case EnvironmentPrecipitationKind::snow:
        return 2.0F;
    case EnvironmentPrecipitationKind::sleet:
        return 3.0F;
    case EnvironmentPrecipitationKind::hail:
        return 4.0F;
    case EnvironmentPrecipitationKind::dust:
        return 5.0F;
    case EnvironmentPrecipitationKind::ash:
        return 6.0F;
    }
    return 0.0F;
}

[[nodiscard]] float first_wetness_intensity(const EnvironmentPrecipitationPlan& environment_plan) noexcept {
    if (environment_plan.wetness_rows.empty()) {
        return 0.0F;
    }
    return environment_plan.wetness_rows.front().intensity;
}

[[nodiscard]] bool valid_environment_particle_rows(const EnvironmentPrecipitationPlan& environment_plan) noexcept {
    return std::ranges::all_of(environment_plan.particle_rows, [](const auto& row) {
        return row.kind != EnvironmentPrecipitationKind::none && finite_in_range(row.intensity, 0.0F, 1.0F) &&
               std::isfinite(row.spawn_rate_per_second) && row.spawn_rate_per_second >= 0.0F &&
               std::isfinite(row.particle_radius_mm) && row.particle_radius_mm > 0.0F &&
               std::isfinite(row.fall_speed_mps) && row.fall_speed_mps >= 0.0F && std::isfinite(row.wind_speed_mps);
    });
}

[[nodiscard]] bool valid_environment_wetness_rows(const EnvironmentPrecipitationPlan& environment_plan) noexcept {
    return std::ranges::all_of(environment_plan.wetness_rows, [](const auto& row) {
        return finite_in_range(row.intensity, 0.0F, 1.0F) && !row.mutates_materials;
    });
}

[[nodiscard]] bool valid_environment_plan(const EnvironmentPrecipitationPlan& environment_plan) noexcept {
    return environment_plan.succeeded() && environment_plan.status == EnvironmentPrecipitationPlanStatus::planned &&
           valid_environment_particle_rows(environment_plan) && valid_environment_wetness_rows(environment_plan) &&
           !environment_plan.invokes_backend && !environment_plan.exposes_native_handles &&
           !environment_plan.mutates_materials && !environment_plan.plays_audio;
}

[[nodiscard]] bool requires_scene_depth_occlusion_readback(const EnvironmentPrecipitationPlan& environment_plan) {
    return std::ranges::any_of(environment_plan.occlusion_rows, [](const auto& row) {
        return row.required && row.available && row.uses_scene_geometry_depth_mask;
    });
}

void append_shader_rows(PrecipitationPolicyPlan& plan, const PrecipitationPolicyDesc& desc) {
    for (const auto& particle : desc.environment_plan.particle_rows) {
        const bool uses_depth = requires_scene_depth_occlusion_readback(desc.environment_plan);
        plan.shader_rows.push_back(PrecipitationShaderRow{
            .kind = particle.kind,
            .constants_binding_slot = precipitation_constants_binding(),
            .particle_texture_binding_slot = precipitation_particle_texture_binding(),
            .scene_depth_texture_binding_slot = precipitation_scene_depth_texture_binding(),
            .sampler_binding_slot = precipitation_sampler_binding(),
            .uses_camera_near_particles = particle.camera_near_only,
            .uses_scene_depth_occlusion = uses_depth,
            .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
        });
    }
}

void append_wetness_rows(PrecipitationPolicyPlan& plan, const PrecipitationPolicyDesc& desc) {
    for (const auto& wetness : desc.environment_plan.wetness_rows) {
        plan.wetness_rows.push_back(PrecipitationSurfaceWetnessPolicyRow{
            .enabled = wetness.enabled,
            .intensity = wetness.intensity,
            .splash_intent = wetness.splash_intent,
            .ripple_intent = wetness.ripple_intent,
            .mutates_materials = false,
        });
    }
}

void append_audio_rows(PrecipitationPolicyPlan& plan, const PrecipitationPolicyDesc& desc) {
    for (const auto& audio : desc.environment_plan.audio_handoff_rows) {
        plan.audio_handoff_rows.push_back(PrecipitationAudioHandoffPolicyRow{
            .cue = audio.cue,
            .intensity = audio.intensity,
            .delay_seconds = audio.delay_seconds,
            .handoff_only = true,
        });
    }
}

void append_quality_row(PrecipitationPolicyPlan& plan, const PrecipitationPolicyDesc& desc) {
    const bool uses_depth =
        std::ranges::any_of(plan.shader_rows, [](const auto& row) { return row.uses_scene_depth_occlusion; });
    plan.quality_rows.push_back(PrecipitationQualityRow{
        .tier = desc.quality_tier,
        .camera_near_particle_path = !plan.shader_rows.empty(),
        .scene_depth_occlusion_path = uses_depth,
        .ready = plan.ready(),
    });
}

} // namespace

bool PrecipitationPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

bool PrecipitationPolicyPlan::ready() const noexcept {
    return status == PrecipitationPolicyStatus::ready;
}

void pack_precipitation_constants(std::span<std::uint8_t> destination, const PrecipitationPolicyDesc& desc) {
    if (destination.size() < precipitation_constants_byte_size()) {
        throw std::invalid_argument("precipitation constants destination is too small");
    }

    std::ranges::fill(destination, std::uint8_t{0});
    if (desc.environment_plan.particle_rows.empty()) {
        return;
    }

    const auto& particle = desc.environment_plan.particle_rows.front();
    write_f32(destination, 0U, particle.intensity);
    write_f32(destination, 4U, particle.fall_speed_mps);
    write_f32(destination, 8U, particle.wind_speed_mps);
    write_f32(destination, 12U, particle.particle_radius_mm);
    write_f32(destination, 16U, 0.0F);
    write_f32(destination, 20U, first_wetness_intensity(desc.environment_plan));
    write_f32(destination, 24U, 0.25F);
    write_f32(destination, 28U, precipitation_kind_id(particle.kind));
}

PrecipitationPolicyPlan plan_precipitation_policy(const PrecipitationPolicyDesc& desc) {
    PrecipitationPolicyPlan plan{
        .status = PrecipitationPolicyStatus::planned,
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
        .package_evidence_ready = desc.package_evidence_ready,
        .execution_evidence_ready = desc.execution_evidence_ready,
    };

    if (!valid_environment_plan(desc.environment_plan)) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::invalid_environment_plan, "environment_plan",
                       "precipitation renderer policy requires a valid value-only environment precipitation plan");
    }
    if (!valid_quality_tier(desc.quality_tier)) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::unsupported_quality_tier, "quality_tier",
                       "precipitation renderer policy requires a supported quality tier");
    }
    if (!desc.shader_contract_evidence_ready) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::missing_shader_contract_evidence,
                       "shader_contract_evidence_ready",
                       "precipitation renderer policy requires validated shader contract evidence");
    }
    if (!desc.package_evidence_ready) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::missing_package_evidence, "package_evidence_ready",
                       "precipitation renderer policy requires package evidence for referenced particle assets");
    }
    if (desc.request_ready_promotion && !desc.execution_evidence_ready) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::missing_execution_evidence, "execution_evidence_ready",
                       "precipitation ready promotion requires D3D12 readback or package execution evidence");
    }
    if (desc.request_particle_buffer_upload &&
        (!desc.execution_evidence_ready || desc.particle_buffer_upload_count == 0U)) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::unsupported_particle_buffer_upload,
                       "request_particle_buffer_upload",
                       "precipitation particle buffer upload requires positive execution evidence counters");
    }
    if (desc.request_backend_execution &&
        (!desc.execution_evidence_ready || desc.backend_invocation_count == 0U || desc.renderer_draw_count == 0U)) {
        add_diagnostic(
            plan, PrecipitationPolicyDiagnosticCode::unsupported_backend_execution, "request_backend_execution",
            "precipitation backend execution requires positive backend invocation and renderer draw counters");
    }
    if (desc.request_backend_execution && requires_scene_depth_occlusion_readback(desc.environment_plan) &&
        !desc.depth_occlusion_readback_proven) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::unsupported_backend_execution,
                       "depth_occlusion_readback_proven",
                       "precipitation backend execution requires scene-depth occlusion color readback evidence");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access",
                       "precipitation policy planning must not expose native renderer or RHI handles");
    }
    if (desc.request_material_mutation) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::unsupported_material_mutation,
                       "request_material_mutation",
                       "precipitation policy planning may emit wetness intent rows but must not mutate materials");
    }
    if (desc.request_audio_playback) {
        add_diagnostic(plan, PrecipitationPolicyDiagnosticCode::unsupported_audio_playback, "request_audio_playback",
                       "precipitation policy planning may pass audio handoff rows but must not play audio");
    }

    if (!plan.succeeded()) {
        plan.status = PrecipitationPolicyStatus::blocked;
    } else if (desc.request_ready_promotion && desc.execution_evidence_ready) {
        plan.status = PrecipitationPolicyStatus::ready;
        if (desc.request_particle_buffer_upload) {
            plan.uploads_particle_buffers = true;
        }
        if (desc.request_backend_execution) {
            plan.invokes_backend = true;
            plan.renderer_draws = desc.renderer_draw_count;
            plan.depth_occlusion_readback = desc.depth_occlusion_readback_proven;
        }
    }

    if (valid_environment_plan(desc.environment_plan)) {
        append_shader_rows(plan, desc);
        append_wetness_rows(plan, desc);
        append_audio_rows(plan, desc);
    }
    append_quality_row(plan, desc);

    return plan;
}

bool has_precipitation_policy_diagnostic(const PrecipitationPolicyPlan& plan,
                                         PrecipitationPolicyDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
