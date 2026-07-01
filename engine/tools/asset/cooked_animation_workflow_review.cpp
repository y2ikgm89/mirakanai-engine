// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/cooked_animation_workflow_review.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <utility>

namespace mirakana {

std::string_view
cooked_animation_workflow_review_status_label(const CookedAnimationWorkflowReviewStatus status) noexcept {
    switch (status) {
    case CookedAnimationWorkflowReviewStatus::review_required:
        return "review_required";
    case CookedAnimationWorkflowReviewStatus::blocked:
        return "blocked";
    case CookedAnimationWorkflowReviewStatus::ready:
        return "ready";
    }
    return "unknown";
}

namespace {

void add_diagnostic(CookedAnimationWorkflowReviewResult& result, const CookedAnimationWorkflowDiagnosticCode code,
                    std::string message) {
    result.diagnostics.push_back(CookedAnimationWorkflowDiagnostic{.code = code, .message = std::move(message)});
}

[[nodiscard]] bool blank(const std::string_view value) noexcept {
    return value.empty() || std::ranges::all_of(value, [](const unsigned char c) { return std::isspace(c) != 0; });
}

[[nodiscard]] std::string_view expected_row_id(const CookedAnimationWorkflowEvidenceKind kind) noexcept {
    switch (kind) {
    case CookedAnimationWorkflowEvidenceKind::gltf_animation_spec:
        return "cooked_animation.workflow.gltf_spec";
    case CookedAnimationWorkflowEvidenceKind::gltf_quaternion_trs_import:
        return "cooked_animation.workflow.gltf_quaternion_trs_import";
    case CookedAnimationWorkflowEvidenceKind::first_party_source_document:
        return "cooked_animation.workflow.source_document";
    case CookedAnimationWorkflowEvidenceKind::cooked_payload:
        return "cooked_animation.workflow.cooked_payload";
    case CookedAnimationWorkflowEvidenceKind::runtime_package_payload:
        return "cooked_animation.workflow.runtime_payload";
    case CookedAnimationWorkflowEvidenceKind::generated_package_smoke:
        return "cooked_animation.workflow.generated_package_smoke";
    case CookedAnimationWorkflowEvidenceKind::clean_room_legal_boundary:
        return "cooked_animation.workflow.clean_room_legal_boundary";
    }
    return "";
}

[[nodiscard]] std::string_view expected_official_source_id(const CookedAnimationWorkflowEvidenceKind kind) noexcept {
    switch (kind) {
    case CookedAnimationWorkflowEvidenceKind::gltf_animation_spec:
        return "khronos-gltf-2.0-spec-animation";
    case CookedAnimationWorkflowEvidenceKind::gltf_quaternion_trs_import:
        return "mirakanai-gltf-node-animation-import";
    case CookedAnimationWorkflowEvidenceKind::first_party_source_document:
        return "mirakanai-animation-quaternion-clip-source-v1";
    case CookedAnimationWorkflowEvidenceKind::cooked_payload:
        return "mirakanai-cooked-animation-quaternion-clip-v1";
    case CookedAnimationWorkflowEvidenceKind::runtime_package_payload:
        return "mirakanai-runtime-animation-quaternion-clip-payload";
    case CookedAnimationWorkflowEvidenceKind::generated_package_smoke:
        return "mirakanai-generated-3d-quaternion-package-smoke";
    case CookedAnimationWorkflowEvidenceKind::clean_room_legal_boundary:
        return "mirakanai-clean-room-legal-boundary";
    }
    return "";
}

void count_feature(CookedAnimationWorkflowReviewResult& result, const CookedAnimationWorkflowEvidenceKind kind) {
    switch (kind) {
    case CookedAnimationWorkflowEvidenceKind::gltf_animation_spec:
        ++result.gltf_spec_rows;
        break;
    case CookedAnimationWorkflowEvidenceKind::gltf_quaternion_trs_import:
        ++result.import_rows;
        break;
    case CookedAnimationWorkflowEvidenceKind::first_party_source_document:
        ++result.source_document_rows;
        break;
    case CookedAnimationWorkflowEvidenceKind::cooked_payload:
        ++result.cooked_payload_rows;
        break;
    case CookedAnimationWorkflowEvidenceKind::runtime_package_payload:
        ++result.runtime_payload_rows;
        break;
    case CookedAnimationWorkflowEvidenceKind::generated_package_smoke:
        ++result.package_smoke_rows;
        break;
    case CookedAnimationWorkflowEvidenceKind::clean_room_legal_boundary:
        ++result.legal_boundary_rows;
        break;
    }
}

[[nodiscard]] bool has_blocked_claims(const CookedAnimationWorkflowEvidenceRow& row) noexcept {
    return row.claims_runtime_gltf_parsing_ready || row.claims_animation_graph_ready || row.claims_retargeting_ready ||
           row.claims_renderer_rhi_execution_ready || row.claims_broad_skeletal_animation_ready ||
           row.native_handles_exposed || row.claims_copied_external_engine_code_assets_or_shaders ||
           row.claims_unity_unreal_godot_compatibility || row.claims_legal_approval;
}

void validate_blocked_claims(CookedAnimationWorkflowReviewResult& result,
                             const CookedAnimationWorkflowEvidenceRow& row) {
    if (row.claims_runtime_gltf_parsing_ready) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::runtime_gltf_parsing_claim_not_allowed,
                       "Cooked animation workflow review must not claim runtime glTF parsing readiness");
    }
    if (row.claims_animation_graph_ready) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::animation_graph_claim_not_allowed,
                       "Cooked animation workflow review must not claim animation graph readiness");
    }
    if (row.claims_retargeting_ready) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::retargeting_claim_not_allowed,
                       "Cooked animation workflow review must not claim retargeting readiness");
    }
    if (row.claims_renderer_rhi_execution_ready) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::renderer_rhi_execution_claim_not_allowed,
                       "Cooked animation workflow review must not claim renderer/RHI execution readiness");
    }
    if (row.claims_broad_skeletal_animation_ready) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::broad_skeletal_animation_claim_not_allowed,
                       "Cooked animation workflow review must not claim broad skeletal animation readiness");
    }
    if (row.native_handles_exposed) {
        result.cooked_animation_native_handles_exposed = true;
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::native_handle_exposure,
                       "Cooked animation workflow review rows must not expose native handles");
    }
    if (row.claims_copied_external_engine_code_assets_or_shaders) {
        add_diagnostic(
            result, CookedAnimationWorkflowDiagnosticCode::copied_external_material_claim_not_allowed,
            "Cooked animation workflow review must not claim copied external engine code, assets, or shaders");
    }
    if (row.claims_unity_unreal_godot_compatibility) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::external_engine_claim_not_allowed,
                       "Cooked animation workflow review must not claim Unity, Unreal, or Godot compatibility");
    }
    if (row.claims_legal_approval) {
        result.cooked_animation_legal_approval = true;
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::legal_approval_claim_not_allowed,
                       "Cooked animation workflow review is engineering evidence and must not claim legal approval");
    }
}

[[nodiscard]] bool validate_common(CookedAnimationWorkflowReviewResult& result,
                                   const CookedAnimationWorkflowEvidenceRow& row,
                                   std::unordered_set<std::string>& seen_row_ids) {
    bool valid = true;
    if (blank(row.row_id)) {
        valid = false;
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_row_id,
                       "Cooked animation workflow evidence rows require stable row ids");
    } else {
        if (row.row_id != expected_row_id(row.kind)) {
            valid = false;
            add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::invalid_row_id,
                           "Cooked animation workflow evidence row id does not match its required row kind");
        }
        if (!seen_row_ids.insert(row.row_id).second) {
            valid = false;
            add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::duplicate_row_id,
                           "Cooked animation workflow evidence row ids must be unique");
        }
    }
    if (blank(row.official_source_id)) {
        valid = false;
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_official_source,
                       "Cooked animation workflow evidence rows require official or repository source ids");
    } else if (row.official_source_id != expected_official_source_id(row.kind)) {
        valid = false;
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::unexpected_official_source,
                       "Cooked animation workflow evidence official source id does not match its required row kind");
    }
    if (!row.reviewed) {
        valid = false;
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_review,
                       "Cooked animation workflow evidence rows must be reviewed");
    }
    return valid;
}

[[nodiscard]] bool require_flag(CookedAnimationWorkflowReviewResult& result, const bool value,
                                const CookedAnimationWorkflowDiagnosticCode code, std::string message) {
    if (value) {
        return true;
    }
    add_diagnostic(result, code, std::move(message));
    return false;
}

[[nodiscard]] bool validate_gltf_spec(CookedAnimationWorkflowReviewResult& result,
                                      const CookedAnimationWorkflowEvidenceRow& row) {
    bool valid = true;
    valid = require_flag(result, row.gltf_20_animation_spec_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_gltf_animation_spec_review,
                         "glTF animation workflow rows require Khronos glTF 2.0 animation specification review") &&
            valid;
    valid = require_flag(result, row.gltf_translation_rotation_scale_paths_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_gltf_trs_channel_review,
                         "glTF animation workflow rows require translation/rotation/scale channel target review") &&
            valid;
    valid = require_flag(result, row.gltf_sampler_interpolation_modes_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_gltf_sampler_interpolation_review,
                         "glTF animation workflow rows require sampler interpolation mode review") &&
            valid;
    valid = require_flag(result, row.gltf_input_time_accessor_min_max_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_gltf_time_accessor_review,
                         "glTF animation workflow rows require input time accessor min/max and key range review") &&
            valid;
    valid = require_flag(result, row.gltf_quaternion_xyzw_rotation_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_gltf_quaternion_review,
                         "glTF animation workflow rows require XYZW quaternion rotation review") &&
            valid;
    valid = require_flag(result, row.gltf_clamped_time_range_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_gltf_clamp_review,
                         "glTF animation workflow rows require clamped output behavior review") &&
            valid;
    valid = require_flag(result, row.gltf_duplicate_target_rejection_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_duplicate_target_review,
                         "glTF animation workflow rows require duplicate target rejection review") &&
            valid;
    valid = require_flag(result, row.gltf_morph_weights_separate_path_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_morph_weights_separate_path_review,
                         "glTF animation workflow rows require morph weights to remain on the separate morph path") &&
            valid;
    return valid;
}

[[nodiscard]] bool validate_import(CookedAnimationWorkflowReviewResult& result,
                                   const CookedAnimationWorkflowEvidenceRow& row) {
    bool valid = true;
    valid = require_flag(result, row.explicit_mesh_preset_required,
                         CookedAnimationWorkflowDiagnosticCode::missing_explicit_mesh_preset_review,
                         "Cooked animation workflow import review requires an explicit mesh preset row") &&
            valid;
    valid = require_flag(result, row.coordinate_normalization_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_coordinate_normalization_review,
                         "Cooked animation workflow import review requires coordinate normalization evidence") &&
            valid;
    valid = require_flag(result, row.unit_quaternion_validation_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_unit_quaternion_validation_review,
                         "Cooked animation workflow import review requires unit-quaternion validation evidence") &&
            valid;
    valid = require_flag(
                result, row.unsupported_interpolation_fails_closed,
                CookedAnimationWorkflowDiagnosticCode::missing_unsupported_interpolation_fail_closed_review,
                "Cooked animation workflow import review requires unsupported interpolation fail-closed evidence") &&
            valid;
    valid =
        require_flag(result, row.no_runtime_gltf_parser_required,
                     CookedAnimationWorkflowDiagnosticCode::missing_no_runtime_gltf_parser_review,
                     "Cooked animation workflow import review must keep runtime glTF parsing out of the ready path") &&
        valid;
    return valid;
}

[[nodiscard]] bool validate_source_document(CookedAnimationWorkflowReviewResult& result,
                                            const CookedAnimationWorkflowEvidenceRow& row) {
    bool valid = true;
    valid = require_flag(result, row.animation_quaternion_clip_source_v1_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_source_document_review,
                         "Cooked animation workflow requires GameEngine.AnimationQuaternionClipSource.v1 review") &&
            valid;
    valid = require_flag(result, row.source_document_validation_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_source_document_validation_review,
                         "Cooked animation workflow requires source document validation review") &&
            valid;
    valid = require_flag(result, row.track_target_joint_rows_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_track_target_joint_review,
                         "Cooked animation workflow requires target and joint-index row review") &&
            valid;
    return valid;
}

[[nodiscard]] bool validate_cooked_payload(CookedAnimationWorkflowReviewResult& result,
                                           const CookedAnimationWorkflowEvidenceRow& row) {
    bool valid = true;
    valid = require_flag(result, row.cooked_animation_quaternion_clip_v1_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_cooked_payload_review,
                         "Cooked animation workflow requires GameEngine.CookedAnimationQuaternionClip.v1 review") &&
            valid;
    valid = require_flag(result, row.asset_kind_animation_quaternion_clip_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_asset_kind_review,
                         "Cooked animation workflow requires AssetKind::animation_quaternion_clip review") &&
            valid;
    valid = require_flag(result, row.deterministic_f32_payload_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_f32_payload_review,
                         "Cooked animation workflow requires deterministic float32 byte payload review") &&
            valid;
    return valid;
}

[[nodiscard]] bool validate_runtime_payload(CookedAnimationWorkflowReviewResult& result,
                                            const CookedAnimationWorkflowEvidenceRow& row) {
    bool valid = true;
    valid = require_flag(result, row.runtime_payload_reader_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_runtime_payload_review,
                         "Cooked animation workflow requires runtime_animation_quaternion_clip_payload review") &&
            valid;
    valid = require_flag(result, row.runtime_package_record_kind_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_runtime_package_record_review,
                         "Cooked animation workflow requires runtime package record kind review") &&
            valid;
    valid = require_flag(result, row.no_runtime_gltf_parser_required,
                         CookedAnimationWorkflowDiagnosticCode::missing_no_runtime_gltf_parser_review,
                         "Cooked animation workflow runtime rows must prove no runtime glTF parsing") &&
            valid;
    return valid;
}

[[nodiscard]] bool validate_package_smoke(CookedAnimationWorkflowReviewResult& result,
                                          const CookedAnimationWorkflowEvidenceRow& row) {
    bool valid = true;
    valid = require_flag(result, row.generated_desktop_3d_package_smoke_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_generated_package_smoke_review,
                         "Cooked animation workflow requires generated DesktopRuntime3DPackage smoke review") &&
            valid;
    valid = require_flag(result, row.pose_sampling_counters_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_pose_sampling_review,
                         "Cooked animation workflow requires pose sampling counter review") &&
            valid;
    valid = require_flag(result, row.deterministic_replay_counter_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_replay_counter_review,
                         "Cooked animation workflow requires deterministic replay counter review") &&
            valid;
    return valid;
}

[[nodiscard]] bool validate_legal_boundary(CookedAnimationWorkflowReviewResult& result,
                                           const CookedAnimationWorkflowEvidenceRow& row) {
    bool valid = true;
    valid = require_flag(result, row.source_provenance_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_source_provenance_review,
                         "Cooked animation workflow requires source provenance review") &&
            valid;
    valid = require_flag(result, row.external_engine_non_claims_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_external_engine_non_claim_review,
                         "Cooked animation workflow requires Unity/Unreal/Godot non-claim review") &&
            valid;
    valid = require_flag(result, row.no_copied_external_material_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_no_copied_external_material_review,
                         "Cooked animation workflow requires no copied external code/assets/shaders review") &&
            valid;
    valid = require_flag(result, row.legal_approval_non_claim_reviewed,
                         CookedAnimationWorkflowDiagnosticCode::missing_legal_approval_non_claim_review,
                         "Cooked animation workflow requires legal-approval non-claim review") &&
            valid;
    return valid;
}

[[nodiscard]] bool validate_feature(CookedAnimationWorkflowReviewResult& result,
                                    const CookedAnimationWorkflowEvidenceRow& row) {
    switch (row.kind) {
    case CookedAnimationWorkflowEvidenceKind::gltf_animation_spec:
        return validate_gltf_spec(result, row);
    case CookedAnimationWorkflowEvidenceKind::gltf_quaternion_trs_import:
        return validate_import(result, row);
    case CookedAnimationWorkflowEvidenceKind::first_party_source_document:
        return validate_source_document(result, row);
    case CookedAnimationWorkflowEvidenceKind::cooked_payload:
        return validate_cooked_payload(result, row);
    case CookedAnimationWorkflowEvidenceKind::runtime_package_payload:
        return validate_runtime_payload(result, row);
    case CookedAnimationWorkflowEvidenceKind::generated_package_smoke:
        return validate_package_smoke(result, row);
    case CookedAnimationWorkflowEvidenceKind::clean_room_legal_boundary:
        return validate_legal_boundary(result, row);
    }
    return false;
}

} // namespace

CookedAnimationWorkflowReviewResult
review_cooked_animation_asset_workflow(const CookedAnimationWorkflowReviewDesc& desc) {
    CookedAnimationWorkflowReviewResult result;
    result.total_rows = desc.rows.size();

    bool has_gltf_spec = false;
    bool has_import = false;
    bool has_source_document = false;
    bool has_cooked_payload = false;
    bool has_runtime_payload = false;
    bool has_package_smoke = false;
    bool has_legal_boundary = false;
    bool blocked = false;
    std::unordered_set<std::string> seen_row_ids;

    for (const auto& row : desc.rows) {
        if (row.reviewed) {
            ++result.reviewed_rows;
        }
        count_feature(result, row.kind);
        validate_blocked_claims(result, row);

        const bool row_blocked = has_blocked_claims(row);
        const bool row_valid = validate_common(result, row, seen_row_ids) && validate_feature(result, row);
        if (row_blocked) {
            ++result.blocked_rows;
            blocked = true;
            continue;
        }
        if (!row_valid) {
            ++result.review_required_rows;
            continue;
        }

        ++result.ready_rows;
        has_gltf_spec = has_gltf_spec || row.kind == CookedAnimationWorkflowEvidenceKind::gltf_animation_spec;
        has_import = has_import || row.kind == CookedAnimationWorkflowEvidenceKind::gltf_quaternion_trs_import;
        has_source_document =
            has_source_document || row.kind == CookedAnimationWorkflowEvidenceKind::first_party_source_document;
        has_cooked_payload = has_cooked_payload || row.kind == CookedAnimationWorkflowEvidenceKind::cooked_payload;
        has_runtime_payload =
            has_runtime_payload || row.kind == CookedAnimationWorkflowEvidenceKind::runtime_package_payload;
        has_package_smoke =
            has_package_smoke || row.kind == CookedAnimationWorkflowEvidenceKind::generated_package_smoke;
        has_legal_boundary =
            has_legal_boundary || row.kind == CookedAnimationWorkflowEvidenceKind::clean_room_legal_boundary;
    }

    result.cooked_animation_gltf_animation_spec_reviewed = has_gltf_spec;
    result.cooked_animation_gltf_quaternion_import_reviewed = has_import;
    result.cooked_animation_source_document_ready = has_source_document;
    result.cooked_animation_cooked_payload_ready = has_cooked_payload;
    result.cooked_animation_runtime_payload_ready = has_runtime_payload;
    result.cooked_animation_generated_package_smoke_ready = has_package_smoke;
    result.cooked_animation_clean_room_legal_boundary_ready = has_legal_boundary;

    if (!has_gltf_spec) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_gltf_spec_row,
                       "Cooked animation workflow review requires a glTF animation specification row");
    }
    if (!has_import) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_import_row,
                       "Cooked animation workflow review requires a glTF quaternion TRS import row");
    }
    if (!has_source_document) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_source_document_row,
                       "Cooked animation workflow review requires a first-party source document row");
    }
    if (!has_cooked_payload) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_cooked_payload_row,
                       "Cooked animation workflow review requires a cooked payload row");
    }
    if (!has_runtime_payload) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_runtime_payload_row,
                       "Cooked animation workflow review requires a runtime payload row");
    }
    if (!has_package_smoke) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_package_smoke_row,
                       "Cooked animation workflow review requires a generated package smoke row");
    }
    if (!has_legal_boundary) {
        add_diagnostic(result, CookedAnimationWorkflowDiagnosticCode::missing_legal_boundary_row,
                       "Cooked animation workflow review requires a clean-room legal boundary row");
    }

    if (blocked) {
        result.status = CookedAnimationWorkflowReviewStatus::blocked;
        return result;
    }
    if (!result.diagnostics.empty() || !has_gltf_spec || !has_import || !has_source_document || !has_cooked_payload ||
        !has_runtime_payload || !has_package_smoke || !has_legal_boundary) {
        result.status = CookedAnimationWorkflowReviewStatus::review_required;
        return result;
    }

    result.status = CookedAnimationWorkflowReviewStatus::ready;
    result.cooked_animation_asset_workflow_review_ready = true;
    result.cooked_animation_quaternion_clip_workflow_ready = true;
    return result;
}

bool has_cooked_animation_workflow_diagnostic(const CookedAnimationWorkflowReviewResult& result,
                                              const CookedAnimationWorkflowDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const CookedAnimationWorkflowDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace mirakana
