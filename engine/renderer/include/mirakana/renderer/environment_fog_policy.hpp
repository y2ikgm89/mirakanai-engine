// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"
#include "mirakana/renderer/postprocess_policy.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentFogMode : std::uint8_t {
    unknown = 0,
    none,
    exponential_height,
    aerial_perspective,
    exponential_height_with_aerial_perspective,
};

enum class EnvironmentFogDiagnosticCode : std::uint8_t {
    none = 0,
    unsupported_mode,
    invalid_density,
    invalid_height_falloff,
    invalid_height_offset,
    invalid_start_distance,
    invalid_cutoff_distance,
    invalid_max_opacity,
    invalid_sky_affect,
    invalid_anisotropy,
    invalid_color,
    invalid_sample_budget,
    missing_scene_depth,
    missing_shader_contract_evidence,
    unsupported_volumetric_fog,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
};

[[nodiscard]] constexpr std::uint32_t environment_fog_scene_depth_texture_binding() noexcept {
    return 2;
}

[[nodiscard]] constexpr std::uint32_t environment_fog_scene_depth_sampler_binding() noexcept {
    return 3;
}

[[nodiscard]] constexpr std::uint32_t environment_fog_constants_binding() noexcept {
    return 4;
}

[[nodiscard]] constexpr std::size_t environment_fog_constants_byte_size() noexcept {
    return 256;
}

struct EnvironmentFogPolicyDesc {
    EnvironmentFogMode mode{EnvironmentFogMode::unknown};
    float density{0.0F};
    float height_falloff{0.2F};
    float height_offset_m{0.0F};
    float start_distance_m{0.0F};
    float cutoff_distance_m{1000.0F};
    float max_opacity{1.0F};
    float sky_affect{0.0F};
    float directional_inscattering_anisotropy{0.0F};
    Vec3 inscattering_color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    Vec3 directional_inscattering_color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    std::uint32_t sample_step_budget{8};
    bool scene_depth_available{false};
    bool shader_contract_evidence_ready{false};
    bool request_volumetric_fog{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
};

void pack_environment_fog_constants(std::span<std::uint8_t> dst, const EnvironmentFogPolicyDesc& desc);

struct EnvironmentFogConstantLayoutRow {
    std::string name;
    std::uint32_t offset_bytes{0};
    std::uint32_t size_bytes{4};
};

struct EnvironmentFogDepthInputRow {
    std::uint32_t texture_binding_slot{0};
    std::uint32_t sampler_binding_slot{0};
    bool required{true};
    bool available{false};
};

struct EnvironmentFogPassBudgetRow {
    std::uint32_t postprocess_pass_count{0};
    std::uint32_t frame_graph_pass_count{0};
    std::uint32_t frame_graph_barrier_step_budget{0};
    std::uint32_t fullscreen_triangle_count{0};
    std::uint32_t sample_step_budget{0};
};

struct EnvironmentFogPostprocessRow {
    PostprocessEffectKind effect{PostprocessEffectKind::unknown};
    bool uses_scene_depth{false};
};

struct EnvironmentFogDiagnostic {
    EnvironmentFogDiagnosticCode code{EnvironmentFogDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct EnvironmentFogPolicyPlan {
    EnvironmentFogMode mode{EnvironmentFogMode::unknown};
    bool requires_scene_depth{false};
    bool scene_depth_available{false};
    bool requires_shader_contract_evidence{false};
    bool shader_contract_evidence_ready{false};
    bool allocates_froxel_volume{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::vector<EnvironmentFogConstantLayoutRow> constant_layout_rows;
    std::vector<EnvironmentFogDepthInputRow> depth_input_rows;
    std::vector<EnvironmentFogPassBudgetRow> pass_budget_rows;
    std::vector<EnvironmentFogPostprocessRow> postprocess_rows;
    std::vector<EnvironmentFogDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentFogPolicyPlan plan_environment_fog_policy(const EnvironmentFogPolicyDesc& desc);

[[nodiscard]] bool has_environment_fog_diagnostic(const EnvironmentFogPolicyPlan& plan,
                                                  EnvironmentFogDiagnosticCode code) noexcept;

} // namespace mirakana
