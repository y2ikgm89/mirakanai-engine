// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/environment_fog_policy.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr float max_fog_density = 10.0F;
constexpr float max_height_falloff = 100.0F;
constexpr float min_cutoff_distance_m = 10.0F;
constexpr float max_distance_m = 10000000.0F;
constexpr float min_anisotropy = -0.99F;
constexpr float max_anisotropy = 0.99F;
constexpr float max_color_component = 100000.0F;
constexpr std::uint32_t max_sample_step_budget = 1024U;

void add_diagnostic(EnvironmentFogPolicyPlan& plan, EnvironmentFogDiagnosticCode code, std::string field,
                    std::string message) {
    plan.diagnostics.push_back(EnvironmentFogDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool is_active_fog_mode(EnvironmentFogMode mode) noexcept {
    return mode != EnvironmentFogMode::none;
}

[[nodiscard]] bool is_supported_mode(EnvironmentFogMode mode) noexcept {
    switch (mode) {
    case EnvironmentFogMode::none:
    case EnvironmentFogMode::exponential_height:
    case EnvironmentFogMode::aerial_perspective:
    case EnvironmentFogMode::exponential_height_with_aerial_perspective:
        return true;
    case EnvironmentFogMode::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool valid_color(Vec3 value) noexcept {
    return finite_in_range(value.x, 0.0F, max_color_component) && finite_in_range(value.y, 0.0F, max_color_component) &&
           finite_in_range(value.z, 0.0F, max_color_component);
}

void append_constant_layout_rows(EnvironmentFogPolicyPlan& plan) {
    constexpr std::array<std::string_view, 14U> names{
        "density",
        "height_falloff",
        "height_offset_m",
        "start_distance_m",
        "cutoff_distance_m",
        "max_opacity",
        "sky_affect",
        "directional_inscattering_anisotropy",
        "inscattering_r",
        "inscattering_g",
        "inscattering_b",
        "directional_inscattering_r",
        "directional_inscattering_g",
        "directional_inscattering_b",
    };

    for (std::uint32_t index = 0U; index < names.size(); ++index) {
        plan.constant_layout_rows.push_back(EnvironmentFogConstantLayoutRow{
            .name = std::string{names[index]},
            .offset_bytes = index * 4U,
        });
    }
}

void append_depth_input_row(EnvironmentFogPolicyPlan& plan) {
    plan.depth_input_rows.push_back(EnvironmentFogDepthInputRow{
        .texture_binding_slot = environment_fog_scene_depth_texture_binding(),
        .sampler_binding_slot = environment_fog_scene_depth_sampler_binding(),
        .required = plan.requires_scene_depth,
        .available = plan.scene_depth_available,
    });
}

void write_f32(std::span<std::uint8_t> dst, std::size_t offset, float value) {
    std::memcpy(dst.data() + offset, &value, sizeof(float));
}

void append_pass_budget_row(EnvironmentFogPolicyPlan& plan, std::uint32_t sample_step_budget) {
    plan.pass_budget_rows.push_back(EnvironmentFogPassBudgetRow{
        .postprocess_pass_count = 1U,
        .frame_graph_pass_count = 2U,
        .frame_graph_barrier_step_budget = 2U,
        .fullscreen_triangle_count = 1U,
        .sample_step_budget = sample_step_budget,
    });
}

void append_postprocess_row(EnvironmentFogPolicyPlan& plan) {
    plan.postprocess_rows.push_back(EnvironmentFogPostprocessRow{
        .effect = PostprocessEffectKind::fog,
        .uses_scene_depth = plan.requires_scene_depth,
    });
}

} // namespace

bool EnvironmentFogPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

bool EnvironmentFogPolicyPlan::ready() const noexcept {
    return status == EnvironmentFogPolicyStatus::ready;
}

void pack_environment_fog_constants(std::span<std::uint8_t> dst, const EnvironmentFogPolicyDesc& desc) {
    if (dst.size() < environment_fog_constants_byte_size()) {
        throw std::invalid_argument("environment fog constants destination is too small");
    }

    std::ranges::fill(dst, std::uint8_t{0});
    write_f32(dst, 0U, desc.density);
    write_f32(dst, 4U, desc.height_falloff);
    write_f32(dst, 8U, desc.height_offset_m);
    write_f32(dst, 12U, desc.start_distance_m);
    write_f32(dst, 16U, desc.cutoff_distance_m);
    write_f32(dst, 20U, desc.max_opacity);
    write_f32(dst, 24U, desc.sky_affect);
    write_f32(dst, 28U, desc.directional_inscattering_anisotropy);
    write_f32(dst, 32U, desc.inscattering_color.x);
    write_f32(dst, 36U, desc.inscattering_color.y);
    write_f32(dst, 40U, desc.inscattering_color.z);
    write_f32(dst, 44U, desc.directional_inscattering_color.x);
    write_f32(dst, 48U, desc.directional_inscattering_color.y);
    write_f32(dst, 52U, desc.directional_inscattering_color.z);
}

EnvironmentFogPolicyPlan plan_environment_fog_policy(const EnvironmentFogPolicyDesc& desc) {
    EnvironmentFogPolicyPlan plan{
        .status = EnvironmentFogPolicyStatus::planned,
        .mode = desc.mode,
        .requires_scene_depth = is_active_fog_mode(desc.mode),
        .scene_depth_available = desc.scene_depth_available,
        .requires_shader_contract_evidence = is_active_fog_mode(desc.mode),
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
        .execution_evidence_ready = desc.execution_evidence_ready,
        .package_evidence_ready = desc.package_evidence_ready,
    };

    if (!is_supported_mode(desc.mode)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::unsupported_mode, "mode",
                       "environment fog requires a supported height fog or aerial perspective mode");
    }
    if (!finite_in_range(desc.density, 0.0F, max_fog_density)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_density, "density",
                       "environment fog density must be finite and non-negative");
    }
    if (!finite_in_range(desc.height_falloff, 0.0001F, max_height_falloff)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_height_falloff, "height_falloff",
                       "environment fog height falloff must be finite and positive");
    }
    if (!std::isfinite(desc.height_offset_m)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_height_offset, "height_offset_m",
                       "environment fog height offset must be finite");
    }
    if (!finite_in_range(desc.start_distance_m, 0.0F, max_distance_m)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_start_distance, "start_distance_m",
                       "environment fog start distance must be finite and non-negative");
    }
    if (!finite_in_range(desc.cutoff_distance_m, min_cutoff_distance_m, max_distance_m) ||
        desc.cutoff_distance_m <= desc.start_distance_m) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_cutoff_distance, "cutoff_distance_m",
                       "environment fog cutoff distance must be finite and greater than the start distance");
    }
    if (!finite_in_range(desc.max_opacity, 0.0F, 1.0F)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_max_opacity, "max_opacity",
                       "environment fog max opacity must be finite and in [0, 1]");
    }
    if (!finite_in_range(desc.sky_affect, 0.0F, 1.0F)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_sky_affect, "sky_affect",
                       "environment fog sky affect must be finite and in [0, 1]");
    }
    if (!finite_in_range(desc.directional_inscattering_anisotropy, min_anisotropy, max_anisotropy)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_anisotropy, "directional_inscattering_anisotropy",
                       "environment fog directional inscattering anisotropy must be finite and bounded");
    }
    if (!valid_color(desc.inscattering_color) || !valid_color(desc.directional_inscattering_color)) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_color, "inscattering_color",
                       "environment fog colors must be finite, non-negative linear color values");
    }
    if (desc.sample_step_budget == 0U || desc.sample_step_budget > max_sample_step_budget) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::invalid_sample_budget, "sample_step_budget",
                       "environment fog sample step budget must be a non-zero bounded count");
    }
    if (plan.requires_scene_depth && !desc.scene_depth_available) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::missing_scene_depth, "scene_depth_available",
                       "environment fog requires scene depth input evidence");
    }
    if (plan.requires_shader_contract_evidence && !desc.shader_contract_evidence_ready) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::missing_shader_contract_evidence,
                       "shader_contract_evidence_ready", "environment fog requires validated shader contract evidence");
    }
    if (desc.request_ready_promotion && !desc.execution_evidence_ready) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::missing_execution_evidence, "execution_evidence_ready",
                       "environment fog ready promotion requires selected backend execution evidence");
    }
    if (desc.request_ready_promotion && !desc.package_evidence_ready) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::missing_package_evidence, "package_evidence_ready",
                       "environment fog ready promotion requires packaged shader/runtime smoke evidence");
    }
    if (desc.request_volumetric_fog) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::unsupported_volumetric_fog, "request_volumetric_fog",
                       "environment fog policy does not allocate froxel or volumetric fog resources in this slice");
    }
    if (desc.request_backend_execution) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::unsupported_backend_execution, "request_backend_execution",
                       "environment fog policy planning must not invoke renderer or RHI backends in this slice");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentFogDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access",
                       "environment fog policy planning must not expose native renderer or RHI handles");
    }

    if (!plan.succeeded()) {
        plan.status = EnvironmentFogPolicyStatus::blocked;
    } else if (desc.request_ready_promotion && desc.execution_evidence_ready && desc.package_evidence_ready) {
        plan.status = EnvironmentFogPolicyStatus::ready;
    }

    if (plan.requires_shader_contract_evidence) {
        append_constant_layout_rows(plan);
    }
    if (plan.requires_scene_depth) {
        append_depth_input_row(plan);
    }
    if (plan.succeeded() && plan.requires_scene_depth) {
        append_pass_budget_row(plan, desc.sample_step_budget);
        append_postprocess_row(plan);
    }

    return plan;
}

bool has_environment_fog_diagnostic(const EnvironmentFogPolicyPlan& plan, EnvironmentFogDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
