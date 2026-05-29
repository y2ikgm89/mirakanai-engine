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

enum class PostprocessToneMappingEvidenceStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class PostprocessToneMappingOperator : std::uint8_t {
    unknown = 0,
    aces_fitted,
    reinhard,
    neutral,
    filmic,
};

enum class PostprocessColorTransferFunction : std::uint8_t {
    unknown = 0,
    linear_scene,
    srgb,
    sc_rgb,
    pq_hdr10,
};

enum class PostprocessToneMappingEvidenceDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_backend,
    duplicate_required_backend,
    unsupported_backend,
    invalid_chain_id,
    duplicate_backend_chain,
    unsupported_operator,
    invalid_transfer_function,
    invalid_luminance_range,
    invalid_exposure_bias,
    missing_hdr_input,
    missing_color_space_evidence,
    missing_resource_synchronization_evidence,
    missing_shader_validation_evidence,
    missing_backend_validation_evidence,
    missing_host_validation_evidence,
    unsupported_native_handle_claim,
    unsupported_subjective_quality_claim,
    missing_backend_row,
    row_budget_exceeded,
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

struct PostprocessToneMappingEvidenceRow {
    std::string chain_id;
    rhi::BackendKind backend{rhi::BackendKind::null};
    PostprocessToneMappingOperator tone_mapping_operator{PostprocessToneMappingOperator::unknown};
    PostprocessColorTransferFunction input_transfer{PostprocessColorTransferFunction::unknown};
    PostprocessColorTransferFunction output_transfer{PostprocessColorTransferFunction::unknown};
    float exposure_bias_ev{0.0F};
    std::uint32_t paper_white_nits{200U};
    std::uint32_t max_content_nits{1000U};
    std::uint32_t display_max_nits{1000U};
    bool hdr_input_available{false};
    bool color_space_evidence_ready{false};
    bool resource_synchronization_evidence_ready{false};
    bool shader_validation_evidence_ready{false};
    bool backend_validation_evidence_ready{false};
    bool host_validated{false};
    bool host_gate_required{false};
    bool request_native_handle_access{false};
    bool request_subjective_visual_quality_claim{false};
    std::uint32_t source_index{0};
};

struct PostprocessToneMappingEvidenceRequest {
    std::vector<rhi::BackendKind> required_backends;
    std::vector<PostprocessToneMappingEvidenceRow> rows;
    std::size_t row_budget{64U};
    std::uint64_t seed{0U};
};

struct PostprocessToneMappingEvidenceDiagnostic {
    PostprocessToneMappingEvidenceDiagnosticCode code{PostprocessToneMappingEvidenceDiagnosticCode::none};
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string chain_id;
    std::string message;
    std::uint32_t source_index{0};
};

struct PostprocessToneMappingEvidencePlan {
    PostprocessToneMappingEvidenceStatus status{PostprocessToneMappingEvidenceStatus::invalid_request};
    std::vector<rhi::BackendKind> required_backends;
    std::vector<PostprocessToneMappingEvidenceRow> rows;
    std::vector<PostprocessToneMappingEvidenceDiagnostic> diagnostics;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t host_validated_backend_count{0U};
    std::size_t rejected_unsafe_row_count{0U};
    std::uint64_t replay_hash{0U};
    bool d3d12_tone_mapping_ready{false};
    bool vulkan_strict_tone_mapping_ready{false};
    bool metal_tone_mapping_ready{false};
    bool requires_metal_host_evidence{false};
    bool has_metal_host_evidence{false};
    bool invoked_gpu_commands{false};
    bool invoked_native_capture{false};
    bool invoked_crash_upload{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] PostprocessChainPolicyPlan plan_postprocess_chain_policy(const PostprocessChainPolicyDesc& desc);

[[nodiscard]] PostprocessToneMappingEvidencePlan
plan_postprocess_tone_mapping_evidence(const PostprocessToneMappingEvidenceRequest& request);

[[nodiscard]] bool has_postprocess_chain_policy_effect(const PostprocessChainPolicyPlan& plan,
                                                       PostprocessEffectKind kind) noexcept;

[[nodiscard]] bool has_postprocess_chain_policy_diagnostic(const PostprocessChainPolicyPlan& plan,
                                                           PostprocessChainDiagnosticCode code) noexcept;

[[nodiscard]] bool
has_postprocess_tone_mapping_evidence_diagnostic(const PostprocessToneMappingEvidencePlan& plan,
                                                 PostprocessToneMappingEvidenceDiagnosticCode code) noexcept;

} // namespace mirakana
