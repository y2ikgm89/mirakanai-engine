// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/weather.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class PrecipitationQualityTier : std::uint8_t {
    unknown = 0,
    low,
    balanced,
    high,
    custom,
};

enum class PrecipitationPolicyStatus : std::uint8_t {
    blocked = 0,
    planned,
    ready,
};

enum class PrecipitationPolicyDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_environment_plan,
    unsupported_quality_tier,
    missing_shader_contract_evidence,
    missing_package_evidence,
    missing_execution_evidence,
    unsupported_particle_buffer_upload,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
    unsupported_material_mutation,
    unsupported_audio_playback,
};

[[nodiscard]] constexpr std::uint32_t precipitation_particle_texture_binding() noexcept {
    return 8;
}

[[nodiscard]] constexpr std::uint32_t precipitation_scene_depth_texture_binding() noexcept {
    return 9;
}

[[nodiscard]] constexpr std::uint32_t precipitation_sampler_binding() noexcept {
    return 8;
}

[[nodiscard]] constexpr std::uint32_t precipitation_constants_binding() noexcept {
    return 7;
}

struct PrecipitationPolicyDesc {
    EnvironmentPrecipitationPlan environment_plan;
    PrecipitationQualityTier quality_tier{PrecipitationQualityTier::balanced};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool request_ready_promotion{false};
    bool request_particle_buffer_upload{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
    bool request_material_mutation{false};
    bool request_audio_playback{false};
};

struct PrecipitationShaderRow {
    EnvironmentPrecipitationKind kind{EnvironmentPrecipitationKind::none};
    std::uint32_t constants_binding_slot{0};
    std::uint32_t particle_texture_binding_slot{0};
    std::uint32_t scene_depth_texture_binding_slot{0};
    std::uint32_t sampler_binding_slot{0};
    bool uses_camera_near_particles{false};
    bool uses_scene_depth_occlusion{false};
    bool shader_contract_evidence_ready{false};
};

struct PrecipitationSurfaceWetnessPolicyRow {
    bool enabled{false};
    float intensity{0.0F};
    bool splash_intent{false};
    bool ripple_intent{false};
    bool mutates_materials{false};
};

struct PrecipitationAudioHandoffPolicyRow {
    EnvironmentPrecipitationAudioCueKind cue{EnvironmentPrecipitationAudioCueKind::unknown};
    float intensity{0.0F};
    float delay_seconds{0.0F};
    bool handoff_only{true};
};

struct PrecipitationQualityRow {
    PrecipitationQualityTier tier{PrecipitationQualityTier::unknown};
    bool camera_near_particle_path{false};
    bool scene_depth_occlusion_path{false};
    bool ready{false};
};

struct PrecipitationPolicyDiagnostic {
    PrecipitationPolicyDiagnosticCode code{PrecipitationPolicyDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct PrecipitationPolicyPlan {
    PrecipitationPolicyStatus status{PrecipitationPolicyStatus::blocked};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool uploads_particle_buffers{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool mutates_materials{false};
    bool plays_audio{false};
    std::vector<PrecipitationShaderRow> shader_rows;
    std::vector<PrecipitationSurfaceWetnessPolicyRow> wetness_rows;
    std::vector<PrecipitationAudioHandoffPolicyRow> audio_handoff_rows;
    std::vector<PrecipitationQualityRow> quality_rows;
    std::vector<PrecipitationPolicyDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] PrecipitationPolicyPlan plan_precipitation_policy(const PrecipitationPolicyDesc& desc);

[[nodiscard]] bool has_precipitation_policy_diagnostic(const PrecipitationPolicyPlan& plan,
                                                       PrecipitationPolicyDiagnosticCode code) noexcept;

} // namespace mirakana
