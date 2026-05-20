// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/postprocess_policy.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool is_supported_effect(PostprocessEffectKind kind) noexcept {
    switch (kind) {
    case PostprocessEffectKind::tone_mapping:
    case PostprocessEffectKind::exposure:
    case PostprocessEffectKind::bloom:
    case PostprocessEffectKind::color_grading:
    case PostprocessEffectKind::fog:
    case PostprocessEffectKind::anti_aliasing:
        return true;
    case PostprocessEffectKind::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] std::string_view effect_name(PostprocessEffectKind kind) noexcept {
    switch (kind) {
    case PostprocessEffectKind::tone_mapping:
        return "tone_mapping";
    case PostprocessEffectKind::exposure:
        return "exposure";
    case PostprocessEffectKind::bloom:
        return "bloom";
    case PostprocessEffectKind::color_grading:
        return "color_grading";
    case PostprocessEffectKind::fog:
        return "fog";
    case PostprocessEffectKind::anti_aliasing:
        return "anti_aliasing";
    case PostprocessEffectKind::unknown:
        return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string_view backend_name(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::null:
        return "null";
    case rhi::BackendKind::d3d12:
        return "d3d12";
    case rhi::BackendKind::vulkan:
        return "vulkan";
    case rhi::BackendKind::metal:
        return "metal";
    }
    return "unknown";
}

void add_diagnostic(PostprocessChainPolicyPlan& plan, PostprocessChainDiagnosticCode code, std::size_t effect_index,
                    std::uint32_t source_index, std::string message) {
    plan.diagnostics.push_back(PostprocessChainDiagnostic{
        .code = code,
        .effect_index = effect_index,
        .source_index = source_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool uses_depth(const PostprocessEffectDesc& effect) noexcept {
    return effect.requires_scene_depth || effect.kind == PostprocessEffectKind::fog;
}

[[nodiscard]] bool has_duplicate_kind(const std::vector<PostprocessEffectRow>& rows,
                                      PostprocessEffectKind kind) noexcept {
    return std::ranges::any_of(rows, [kind](const PostprocessEffectRow& row) { return row.kind == kind; });
}

[[nodiscard]] bool is_supported_anti_aliasing(PostprocessAntiAliasingMode mode) noexcept {
    return mode == PostprocessAntiAliasingMode::none || mode == PostprocessAntiAliasingMode::fxaa;
}

} // namespace

PostprocessChainPolicyPlan plan_postprocess_chain_policy(const PostprocessChainPolicyDesc& desc) {
    PostprocessChainPolicyPlan plan;
    plan.frame_extent = desc.frame_extent;
    plan.backend = desc.backend;
    plan.backend_shader_evidence_ready = desc.backend_shader_evidence_ready;
    const auto enabled_effect_count = static_cast<std::uint32_t>(
        std::ranges::count_if(desc.effects, [](const PostprocessEffectDesc& effect) { return effect.enabled; }));
    plan.scene_color_required = enabled_effect_count != 0;

    if (desc.frame_extent.width == 0 || desc.frame_extent.height == 0) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::invalid_frame_extent, 0, 0,
                       "postprocess chain policy requires a non-zero frame extent");
    }
    if (enabled_effect_count == 0) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::no_effects, 0, 0,
                       "postprocess chain policy requires at least one enabled effect");
    }
    if (desc.max_effect_count == 0) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::too_many_effects, 0, 0,
                       "postprocess chain policy max_effect_count must be greater than zero");
    }
    if (desc.max_postprocess_pass_count == 0) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::too_many_postprocess_passes, 0, 0,
                       "postprocess chain policy max_postprocess_pass_count must be greater than zero");
    }
    if (enabled_effect_count > desc.max_effect_count) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::too_many_effects, enabled_effect_count, 0,
                       "postprocess chain policy exceeds max_effect_count");
    }
    if (plan.scene_color_required && !desc.scene_color_available) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::missing_scene_color, 0, 0,
                       "postprocess chain policy requires scene_color before postprocess execution");
    }
    if (desc.require_backend_shader_evidence && !desc.backend_shader_evidence_ready) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::missing_backend_shader_evidence, 0, 0,
                       std::string{"postprocess chain policy requires shader evidence for "} +
                           std::string{backend_name(desc.backend)});
    }

    bool bloom_requested = false;
    for (std::size_t index = 0; index < desc.effects.size(); ++index) {
        const auto& effect = desc.effects[index];
        if (!effect.enabled) {
            continue;
        }

        if (!is_supported_effect(effect.kind)) {
            add_diagnostic(plan, PostprocessChainDiagnosticCode::unsupported_effect, index, effect.source_index,
                           std::string{"unsupported postprocess effect "} + std::string{effect_name(effect.kind)});
            continue;
        }
        if (has_duplicate_kind(plan.effect_rows, effect.kind)) {
            add_diagnostic(plan, PostprocessChainDiagnosticCode::duplicate_effect, index, effect.source_index,
                           std::string{"duplicate postprocess effect "} + std::string{effect_name(effect.kind)});
            continue;
        }
        if (!std::isfinite(effect.intensity) || effect.intensity < 0.0F) {
            add_diagnostic(plan, PostprocessChainDiagnosticCode::invalid_effect_intensity, index, effect.source_index,
                           std::string{"postprocess effect "} + std::string{effect_name(effect.kind)} +
                               " requires finite non-negative intensity");
            continue;
        }
        if (effect.kind == PostprocessEffectKind::bloom) {
            if (effect.bloom_iterations == 0 || effect.bloom_iterations > 8) {
                add_diagnostic(plan, PostprocessChainDiagnosticCode::invalid_bloom_iterations, index,
                               effect.source_index, "postprocess bloom requires bloom_iterations in the range 1..8");
                continue;
            }
            bloom_requested = true;
        }
        if (effect.kind == PostprocessEffectKind::anti_aliasing && !is_supported_anti_aliasing(effect.anti_aliasing)) {
            add_diagnostic(plan, PostprocessChainDiagnosticCode::unsupported_anti_aliasing_mode, index,
                           effect.source_index, "postprocess anti-aliasing currently supports none or fxaa only");
            continue;
        }

        const bool depth_required = uses_depth(effect);
        if (depth_required) {
            plan.scene_depth_required = true;
            if (!desc.scene_depth_available) {
                add_diagnostic(plan, PostprocessChainDiagnosticCode::missing_scene_depth, index, effect.source_index,
                               std::string{"postprocess effect "} + std::string{effect_name(effect.kind)} +
                                   " requires scene depth input");
            }
        }

        plan.effect_rows.push_back(PostprocessEffectRow{
            .kind = effect.kind,
            .intensity = effect.intensity,
            .bloom_iterations = effect.kind == PostprocessEffectKind::bloom ? effect.bloom_iterations : 0,
            .anti_aliasing = effect.kind == PostprocessEffectKind::anti_aliasing ? effect.anti_aliasing
                                                                                 : PostprocessAntiAliasingMode::none,
            .uses_scene_depth = depth_required,
            .pass_index = 0,
            .source_index = effect.source_index,
        });
    }

    plan.effect_count = static_cast<std::uint32_t>(plan.effect_rows.size());

    plan.bloom_work_texture_required = bloom_requested;
    plan.postprocess_pass_count = plan.effect_count == 0 ? 0U : bloom_requested ? 2U : 1U;
    if (plan.postprocess_pass_count > desc.max_postprocess_pass_count) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::too_many_postprocess_passes, 0, 0,
                       "postprocess chain policy exceeds max_postprocess_pass_count");
    }
    if (plan.postprocess_pass_count != 0) {
        plan.frame_graph_pass_count = 1U + plan.postprocess_pass_count;
        plan.frame_graph_barrier_step_budget = plan.postprocess_pass_count * 2U;
    }
    if (plan.postprocess_pass_count > 1U) {
        for (auto& row : plan.effect_rows) {
            if (row.kind == PostprocessEffectKind::anti_aliasing || row.kind == PostprocessEffectKind::color_grading) {
                row.pass_index = plan.postprocess_pass_count - 1U;
            }
        }
    }
    return plan;
}

bool has_postprocess_chain_policy_effect(const PostprocessChainPolicyPlan& plan, PostprocessEffectKind kind) noexcept {
    return std::ranges::any_of(plan.effect_rows, [kind](const PostprocessEffectRow& row) { return row.kind == kind; });
}

bool has_postprocess_chain_policy_diagnostic(const PostprocessChainPolicyPlan& plan,
                                             PostprocessChainDiagnosticCode code) noexcept {
    return std::ranges::any_of(
        plan.diagnostics, [code](const PostprocessChainDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
