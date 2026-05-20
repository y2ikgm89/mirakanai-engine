// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class PostprocessEffectKind : std::uint8_t {
    unknown = 0,
    tone_mapping,
    exposure,
    bloom,
    color_grading,
    fog,
    anti_aliasing,
};

enum class PostprocessAntiAliasingMode : std::uint8_t {
    none = 0,
    fxaa,
    taa,
};

enum class PostprocessChainDiagnosticCode : std::uint8_t {
    none = 0,
    no_effects,
    invalid_frame_extent,
    missing_scene_color,
    missing_scene_depth,
    too_many_effects,
    too_many_postprocess_passes,
    unsupported_effect,
    duplicate_effect,
    invalid_effect_intensity,
    invalid_bloom_iterations,
    unsupported_anti_aliasing_mode,
    missing_backend_shader_evidence,
};

struct PostprocessEffectDesc {
    PostprocessEffectKind kind{PostprocessEffectKind::unknown};
    bool enabled{true};
    float intensity{1.0F};
    std::uint32_t bloom_iterations{1};
    PostprocessAntiAliasingMode anti_aliasing{PostprocessAntiAliasingMode::none};
    bool requires_scene_depth{false};
    std::uint32_t source_index{0};
};

struct PostprocessChainPolicyDesc {
    std::span<const PostprocessEffectDesc> effects;
    Extent2D frame_extent;
    bool scene_color_available{false};
    bool scene_depth_available{false};
    std::uint32_t max_effect_count{6};
    std::uint32_t max_postprocess_pass_count{2};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool require_backend_shader_evidence{false};
    bool backend_shader_evidence_ready{false};
};

struct PostprocessEffectRow {
    PostprocessEffectKind kind{PostprocessEffectKind::unknown};
    float intensity{1.0F};
    std::uint32_t bloom_iterations{0};
    PostprocessAntiAliasingMode anti_aliasing{PostprocessAntiAliasingMode::none};
    bool uses_scene_depth{false};
    std::uint32_t pass_index{0};
    std::uint32_t source_index{0};
};

struct PostprocessChainDiagnostic {
    PostprocessChainDiagnosticCode code{PostprocessChainDiagnosticCode::none};
    std::size_t effect_index{0};
    std::uint32_t source_index{0};
    std::string message;
};

struct PostprocessChainPolicyPlan {
    std::uint32_t effect_count{0};
    std::uint32_t postprocess_pass_count{0};
    std::uint32_t frame_graph_pass_count{0};
    std::uint32_t frame_graph_barrier_step_budget{0};
    Extent2D frame_extent;
    bool scene_color_required{false};
    bool scene_depth_required{false};
    bool bloom_work_texture_required{false};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool backend_shader_evidence_ready{false};
    std::vector<PostprocessEffectRow> effect_rows;
    std::vector<PostprocessChainDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] PostprocessChainPolicyPlan plan_postprocess_chain_policy(const PostprocessChainPolicyDesc& desc);

[[nodiscard]] bool has_postprocess_chain_policy_effect(const PostprocessChainPolicyPlan& plan,
                                                       PostprocessEffectKind kind) noexcept;

[[nodiscard]] bool has_postprocess_chain_policy_diagnostic(const PostprocessChainPolicyPlan& plan,
                                                           PostprocessChainDiagnosticCode code) noexcept;

} // namespace mirakana
