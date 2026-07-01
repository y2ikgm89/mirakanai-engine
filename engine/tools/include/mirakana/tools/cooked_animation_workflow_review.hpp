// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class CookedAnimationWorkflowEvidenceKind : std::uint8_t {
    gltf_animation_spec,
    gltf_quaternion_trs_import,
    first_party_source_document,
    cooked_payload,
    runtime_package_payload,
    generated_package_smoke,
    clean_room_legal_boundary,
};

enum class CookedAnimationWorkflowReviewStatus : std::uint8_t {
    review_required,
    blocked,
    ready,
};

enum class CookedAnimationWorkflowDiagnosticCode : std::uint8_t {
    none,
    missing_row_id,
    invalid_row_id,
    duplicate_row_id,
    missing_official_source,
    unexpected_official_source,
    missing_review,
    missing_gltf_animation_spec_review,
    missing_gltf_trs_channel_review,
    missing_gltf_sampler_interpolation_review,
    missing_gltf_time_accessor_review,
    missing_gltf_quaternion_review,
    missing_gltf_clamp_review,
    missing_duplicate_target_review,
    missing_morph_weights_separate_path_review,
    missing_explicit_mesh_preset_review,
    missing_coordinate_normalization_review,
    missing_unit_quaternion_validation_review,
    missing_unsupported_interpolation_fail_closed_review,
    missing_no_runtime_gltf_parser_review,
    missing_source_document_review,
    missing_source_document_validation_review,
    missing_track_target_joint_review,
    missing_cooked_payload_review,
    missing_asset_kind_review,
    missing_f32_payload_review,
    missing_runtime_payload_review,
    missing_runtime_package_record_review,
    missing_generated_package_smoke_review,
    missing_pose_sampling_review,
    missing_replay_counter_review,
    missing_source_provenance_review,
    missing_external_engine_non_claim_review,
    missing_no_copied_external_material_review,
    missing_legal_approval_non_claim_review,
    missing_gltf_spec_row,
    missing_import_row,
    missing_source_document_row,
    missing_cooked_payload_row,
    missing_runtime_payload_row,
    missing_package_smoke_row,
    missing_legal_boundary_row,
    runtime_gltf_parsing_claim_not_allowed,
    animation_graph_claim_not_allowed,
    retargeting_claim_not_allowed,
    renderer_rhi_execution_claim_not_allowed,
    broad_skeletal_animation_claim_not_allowed,
    native_handle_exposure,
    copied_external_material_claim_not_allowed,
    external_engine_claim_not_allowed,
    legal_approval_claim_not_allowed,
};

struct CookedAnimationWorkflowDiagnostic {
    CookedAnimationWorkflowDiagnosticCode code{CookedAnimationWorkflowDiagnosticCode::none};
    std::string message;
};

struct CookedAnimationWorkflowEvidenceRow {
    CookedAnimationWorkflowEvidenceKind kind{CookedAnimationWorkflowEvidenceKind::gltf_animation_spec};
    std::string row_id;
    std::string official_source_id;
    bool reviewed{false};
    bool gltf_20_animation_spec_reviewed{false};
    bool gltf_translation_rotation_scale_paths_reviewed{false};
    bool gltf_sampler_interpolation_modes_reviewed{false};
    bool gltf_input_time_accessor_min_max_reviewed{false};
    bool gltf_quaternion_xyzw_rotation_reviewed{false};
    bool gltf_clamped_time_range_reviewed{false};
    bool gltf_duplicate_target_rejection_reviewed{false};
    bool gltf_morph_weights_separate_path_reviewed{false};
    bool explicit_mesh_preset_required{false};
    bool coordinate_normalization_reviewed{false};
    bool unit_quaternion_validation_reviewed{false};
    bool unsupported_interpolation_fails_closed{false};
    bool no_runtime_gltf_parser_required{false};
    bool animation_quaternion_clip_source_v1_reviewed{false};
    bool source_document_validation_reviewed{false};
    bool track_target_joint_rows_reviewed{false};
    bool cooked_animation_quaternion_clip_v1_reviewed{false};
    bool asset_kind_animation_quaternion_clip_reviewed{false};
    bool deterministic_f32_payload_reviewed{false};
    bool runtime_payload_reader_reviewed{false};
    bool runtime_package_record_kind_reviewed{false};
    bool generated_desktop_3d_package_smoke_reviewed{false};
    bool pose_sampling_counters_reviewed{false};
    bool deterministic_replay_counter_reviewed{false};
    bool source_provenance_reviewed{false};
    bool external_engine_non_claims_reviewed{false};
    bool no_copied_external_material_reviewed{false};
    bool legal_approval_non_claim_reviewed{false};
    bool claims_runtime_gltf_parsing_ready{false};
    bool claims_animation_graph_ready{false};
    bool claims_retargeting_ready{false};
    bool claims_renderer_rhi_execution_ready{false};
    bool claims_broad_skeletal_animation_ready{false};
    bool native_handles_exposed{false};
    bool claims_copied_external_engine_code_assets_or_shaders{false};
    bool claims_unity_unreal_godot_compatibility{false};
    bool claims_legal_approval{false};
};

struct CookedAnimationWorkflowReviewDesc {
    std::span<const CookedAnimationWorkflowEvidenceRow> rows;
};

struct CookedAnimationWorkflowReviewResult {
    CookedAnimationWorkflowReviewStatus status{CookedAnimationWorkflowReviewStatus::review_required};
    std::vector<CookedAnimationWorkflowDiagnostic> diagnostics;
    std::size_t total_rows{0};
    std::size_t reviewed_rows{0};
    std::size_t ready_rows{0};
    std::size_t blocked_rows{0};
    std::size_t review_required_rows{0};
    std::size_t gltf_spec_rows{0};
    std::size_t import_rows{0};
    std::size_t source_document_rows{0};
    std::size_t cooked_payload_rows{0};
    std::size_t runtime_payload_rows{0};
    std::size_t package_smoke_rows{0};
    std::size_t legal_boundary_rows{0};
    bool cooked_animation_asset_workflow_review_ready{false};
    bool cooked_animation_quaternion_clip_workflow_ready{false};
    bool cooked_animation_gltf_animation_spec_reviewed{false};
    bool cooked_animation_gltf_quaternion_import_reviewed{false};
    bool cooked_animation_source_document_ready{false};
    bool cooked_animation_cooked_payload_ready{false};
    bool cooked_animation_runtime_payload_ready{false};
    bool cooked_animation_generated_package_smoke_ready{false};
    bool cooked_animation_clean_room_legal_boundary_ready{false};
    bool cooked_animation_runtime_gltf_parsing_ready{false};
    bool cooked_animation_animation_graph_ready{false};
    bool cooked_animation_retargeting_ready{false};
    bool cooked_animation_renderer_rhi_execution_ready{false};
    bool cooked_animation_broad_skeletal_animation_ready{false};
    bool cooked_animation_native_handles_exposed{false};
    bool cooked_animation_external_engine_compatibility{false};
    bool cooked_animation_legal_approval{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == CookedAnimationWorkflowReviewStatus::ready && cooked_animation_asset_workflow_review_ready;
    }
};

[[nodiscard]] std::string_view
cooked_animation_workflow_review_status_label(CookedAnimationWorkflowReviewStatus status) noexcept;
[[nodiscard]] CookedAnimationWorkflowReviewResult
review_cooked_animation_asset_workflow(const CookedAnimationWorkflowReviewDesc& desc);
[[nodiscard]] bool has_cooked_animation_workflow_diagnostic(const CookedAnimationWorkflowReviewResult& result,
                                                            CookedAnimationWorkflowDiagnosticCode code) noexcept;

} // namespace mirakana
